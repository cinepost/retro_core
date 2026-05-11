#include "renderer.h"

#include <omp.h>

#include <cstring>
#include <limits>
#include <thread>


namespace RetroCore {

static const bool kDefaultShoeDebugInfoState = true;
static constexpr uint16_t kMaxFramebufferAxisSize = std::numeric_limits<uint16_t>::max() / 2;
static const uint8_t  kDefaultPixelStride = 4; // 4 bytes per pixel
static const Renderer::CursorType kDefaultDebugCursorType = Renderer::CursorType::HAND;

Renderer::Renderer(): mIsInitialized(false), 
	mIsFramebufferClear(false), 
	mIsExternalFramebufferClear(false),
	mPixelStride(kDefaultPixelStride),
	mShowDebugInfo(kDefaultShoeDebugInfoState),
	mDebugCursorType(kDefaultDebugCursorType) {
}

bool Renderer::init(uint16_t framebuffer_width, uint16_t framebuffer_height) {
	assert(framebuffer_width > 0);
	assert(framebuffer_height > 0);

	mFramebufferWidth = std::min(kMaxFramebufferAxisSize, framebuffer_width);
	mFramebufferHeight = std::min(kMaxFramebufferAxisSize, framebuffer_height);

	mFramebufferStride = mFramebufferWidth * static_cast<uint32_t>(mPixelStride);
	mFramebufferDataSize = mFramebufferStride * mFramebufferHeight;
	mFramebuffer.resize(mFramebufferDataSize);

	reset();

	mIsInitialized = true;

	return mIsInitialized;
}

void Renderer::deinit() {
	if(!isInitialized()) {
		return;
	}
}

void Renderer::clearFramebuffer() {
	if(mIsFramebufferClear) return;
	std::memset(mFramebuffer.data(), 0, mFramebufferDataSize);
	mIsFramebufferClear = true;
}

void Renderer::clearFramebuffer(uint8_t* pFrameData, uint32_t stride_bytes) {
	if(mIsExternalFramebufferClear) return;
	assert(pFrameData);

	uint8_t* pData = reinterpret_cast<uint8_t*>(pFrameData);
	for(size_t i = 0; i < mFramebufferHeight; ++i, pData += stride_bytes) {
		std::memset(pData, 0, stride_bytes);
	}
	mIsExternalFramebufferClear = true;
}

const uint8_t* Renderer::render() {
	const bool result = _render(mFramebuffer.data(), mFramebufferStride, true);
	mIsFramebufferClear = false;
	return mFramebuffer.data();
}

const uint8_t* Renderer::render(uint8_t* pFrameData, uint32_t stride_bytes) {
	assert(pFrameData);
	const bool result = _render(pFrameData, stride_bytes, false);
	mIsExternalFramebufferClear = false;
	return pFrameData;
}

bool Renderer::_render(uint8_t* pFrameData, uint32_t stride_bytes, bool use_internal_buffer) {
	static uint32_t s_square_size = 16;
	if(mShowDebugInfo) {
		drawDebugBackground(pFrameData, stride_bytes, s_square_size);
		drawDebugCursor(pFrameData, stride_bytes);
	}

	s_square_size++;
	if(s_square_size >= 32) s_square_size = 8;

	return renderImpl(pFrameData, stride_bytes);
}

void Renderer::drawDebugBackground(uint8_t* pFrameData, uint32_t stride_bytes, uint32_t square_size, uint32_t color1, uint32_t color2) {
	std::vector<uint32_t> rowA(mFramebufferWidth);
    std::vector<uint32_t> rowB(mFramebufferWidth);
    
    for (int x = 0; x < mFramebufferWidth; ++x) {
        bool col_even = (x / square_size) % 2 == 0;
        rowA[x] = col_even ? color1 : color2;
        rowB[x] = col_even ? color2 : color1;
    }

	#pragma omp parallel for num_threads(2) schedule(static)
    for (int y = 0; y < mFramebufferHeight; ++y) {
        uint8_t* dst = pFrameData + (y * stride_bytes);
        bool use_row_a = (y / square_size) % 2 == 0;
        memcpy(dst, use_row_a ? rowA.data() : rowB.data(), stride_bytes);
    }
}


void Renderer::drawDebugCursor(uint8_t* pFrameData, uint32_t stride_bytes) {
	switch(mDebugCursorType) {
		case CursorType::HAND:
			blit<BlitMode::OVER>(pFrameData, stride_bytes, DebugData::PointerHand.data(), DebugData::PointerHand.width, DebugData::PointerHand.height, mDebugCursorPos.x - DebugData::PointerHand.point_x, mDebugCursorPos.y - DebugData::PointerHand.point_y);
			break;
		case CursorType::CROSSHAIR:
		default:
			blit<BlitMode::XOR>(pFrameData, stride_bytes, DebugData::Crosshair.data(), DebugData::Crosshair.width, DebugData::Crosshair.height, mDebugCursorPos.x - DebugData::Crosshair.point_x, mDebugCursorPos.y - DebugData::Crosshair.point_y);
			break;
	}
	//invertPixel(pFrameData, stride_bytes, mDebugCursorPos.x, mDebugCursorPos.y);
}

template <Renderer::BlitMode M>
void Renderer::blit(uint8_t* pFrameData, uint32_t stride_bytes, const uint8_t* pSrcData, uint16_t src_width, uint16_t src_height, int16_t dst_pos_x, int16_t dst_pos_y) {
	if(dst_pos_x >= mFramebufferWidth || dst_pos_y >= mFramebufferHeight || (dst_pos_x + src_height) < 0 || (dst_pos_y + src_height) < 0) return;

	const uint16_t src_stride_bytes = src_width * mPixelStride;

	uint16_t x_count = src_width, y_count = src_height;
	uint16_t dst_x_start = dst_pos_x, dst_y_start = dst_pos_y;
	uint16_t src_x_start = 0, src_y_start = 0;

	if(dst_pos_x < 0) {
		dst_x_start = 0;
		src_x_start = -dst_pos_x;
		x_count = src_width + dst_pos_x;
	} else if (dst_pos_x + src_width >= mFramebufferWidth) {
		x_count = mFramebufferWidth - dst_pos_x;
	}

	if(dst_pos_y < 0) {
		dst_y_start = 0;
		src_y_start = -dst_pos_y;
		y_count = src_height + dst_pos_y;
	} else if (dst_pos_y + src_height >= mFramebufferHeight) {
		y_count = mFramebufferHeight - dst_pos_y;
	}

	uint8_t* pDstLine = pFrameData + dst_y_start * stride_bytes;
	const uint8_t* pSrcLine = pSrcData + src_y_start * src_stride_bytes;
	for(uint16_t y = 0; y < y_count; ++y, pDstLine += stride_bytes, pSrcLine += src_stride_bytes) {
		uint8_t* p = pDstLine + dst_x_start * mPixelStride;
		const uint8_t* pSrc = pSrcLine + src_x_start * mPixelStride;

		if constexpr (M == BlitMode::REPLACE) {
			std::memcpy(p, pSrc, 3);
		} else {
			for(uint16_t x = 0; x < x_count; ++x, p += mPixelStride, pSrc += mPixelStride) {
				if constexpr (M == BlitMode::XOR) {
					p[0] ^= pSrc[0];
					p[1] ^= pSrc[1];
					p[2] ^= pSrc[2];
				} else if constexpr (M == BlitMode::OVER) {
					// Optimized RGBA SWAR Implementation
					uint32_t src, dst;
					std::memcpy(&src, pSrc, 4);
    				std::memcpy(&dst, p, 4);
				
					uint8_t alpha = pSrc[3]; 
    
					if (alpha == 0) continue;
					if (alpha == 255) {
						std::memcpy(p, pSrc, 4);
						continue;
					}

					// 1. Expand 8-bit channels to 16-bit slots (00RR00GG 00BB00AA)
					uint64_t s64 = (src | ((uint64_t)(src & 0xFF00FF00) << 24)) & 0x00FF00FF00FF00FFULL;
					uint64_t d64 = (dst | ((uint64_t)(dst & 0xFF00FF00) << 24)) & 0x00FF00FF00FF00FFULL;

					// 2. Linear Interpolation: result = d + (alpha * (s - d)) / 255
					// To handle signed (s - d) in SWAR, we use: result = ((s*al) + (d*(255-al))) / 255
					uint64_t inv_al = 255 - alpha;
					uint64_t res = (s64 * alpha) + (d64 * inv_al) + 0x0080008000800080ULL; // Add 128 for rounding

					// 3. Fast division by 255: (x + (x >> 8) + 1) >> 8
					res = (res + ((res >> 8) & 0x00FF00FF00FF00FFULL)) >> 8;
					res &= 0x00FF00FF00FF00FFULL;

					// 4. Pack back to 32-bit (RR GG BB AA)
					uint32_t final = (uint32_t)(res | (res >> 24));
					memcpy(p, &final, 4);
				}
			}
		}
	}
}

void Renderer::invertPixel(uint8_t* pFrameData, uint32_t stride_bytes, uint16_t x, uint16_t y) {
	if(x >= mFramebufferWidth || y >= mFramebufferHeight || x < 0 || y < 0) return;
	uint8_t* p = pFrameData + y * stride_bytes + x * mPixelStride;
	p[0] = ~p[0];
	p[1] = ~p[1];
	p[2] = ~p[2];
}

void Renderer::setDebugBackgroundPos(uint32_t x, uint32_t y) { 
	mDebugBackgroundPos.x = x;
	mDebugBackgroundPos.y = y;
}

void Renderer::setDebugCursorPos(uint32_t x, uint32_t y) { 
	mDebugCursorPos.x = std::max(0u, std::min(mFramebufferWidth - 1u, x));
	mDebugCursorPos.y = std::max(0u, std::min(mFramebufferHeight - 1u, y));
}

void Renderer::moveDebugCursor(int16_t x_delta, int16_t y_delta) {
	mDebugCursorPos.x = std::max(0, std::min((int)mFramebufferWidth - 1,  (int)mDebugCursorPos.x + x_delta));
	mDebugCursorPos.y = std::max(0, std::min((int)mFramebufferHeight - 1, (int)mDebugCursorPos.y + y_delta));
}

void Renderer::reset() {
	clearFramebuffer();
	mShowDebugInfo = kDefaultShoeDebugInfoState;
	mDebugCursorType = kDefaultDebugCursorType;
	mDebugBackgroundPos = Coord();
	mDebugCursorPos = Coord(100, 100);
}

}  // namespace RetroCore