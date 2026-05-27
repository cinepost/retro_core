#include "framework/ppu/ppu_nes.h"

#include <immintrin.h>


namespace RetroCore {

namespace PPU {

static_assert(sizeof(NesPPU_BASE::CHRTile) == 16);
static_assert(sizeof(NesPPU_BASE::PatternTable) == 65536 * 16); // 1mb of CHR tiles data

template <FramebufferDims FBDIMS>
bool NesPPU<FBDIMS>::init() {
	return true;
}

template <FramebufferDims FBDIMS>
bool NesPPU<FBDIMS>::deinit() {
	return true;
}

template <FramebufferDims FBDIMS>
__attribute__((target("avx2")))
inline void blitNativeFramebufferToHost(const uint8_t* __restrict p_indexed_fb, uint32_t* __restrict p_host_fb, const uint32_t* __restrict p_nes_color_lut /* 64 colors RGBA (uint32_t) NES system palette */) {
    constexpr size_t TOTAL_PIXELS = FBDIMS.width * FBDIMS.height;
    
    // Process 8 pixels at a time (8 * 32-bit = 256-bit AVX register)
    for (size_t i = 0; i < TOTAL_PIXELS; i += 8) {
        // Load 8 bytes of indexed pixels into a 64-bit integer
        uint64_t pixel_bytes = *reinterpret_cast<const uint64_t*>(&p_indexed_fb[i]);
        
        // Move to a 128-bit SSE register
        __m128i packed_indices = _mm_cvtsi64_si128(pixel_bytes);
        
        // Zero-extend the 8-bit indices into 32-bit integers inside a 256-bit AVX register
        __m256i indices_32 = _mm256_cvtepu8_epi32(packed_indices);
        
        // Parallel Gather: Collect 8 distinct 32-bit RGBA values from the LUT at once
        __m256i xrgb_pixels = _mm256_i32gather_epi32(
            reinterpret_cast<const int*>(p_nes_color_lut), 
            indices_32, 
            4 // Scale factor (sizeof(uint32_t) = 4 bytes)
        );
        
        // Stream the values directly into the host framebuffer
        // If host FB is aligned to 32 bytes, use _mm256_store_si256
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(&p_host_fb[i]), xrgb_pixels);
    }
}

template <FramebufferDims FBDIMS>
bool NesPPU<FBDIMS>::render(uint8_t* pFrameData, uint32_t stride_bytes) {
	static_assert(FBDIMS.height > 0);
	const std::lock_guard<std::mutex> lock_frame(mFrameMutex);

	// prepare sprites
	mVisibleSpritesCount = 0;
	if(mSpritesEnabled) {

		UNROLL_64
		for(const Sprite& sprite: mSprites) {
			if((sprite.x >= FBDIMS.width) || ((sprite.x + 8) < 0) || (sprite.y >= FBDIMS.height) || ((sprite.y + 8) < 0)) continue;
			mVisibleSpritesCount++;
		}
	}

	// iterate over scanlines
	UNROLL_64
	for (uint16_t scanline = 0; scanline < FBDIMS.height; ++scanline) {
		if(scanline % 2) continue; 
		uint32_t* pHostLineData = reinterpret_cast<uint32_t*>(pFrameData + scanline * stride_bytes);

		uint16_t scroll_y = mScrollY + scanline;
		uint16_t scroll_x = mScrollX;
		        
        // Per-scanline callback if registered one
        if (mScanlineCallback != nullptr) {
            mScanlineCallback(scanline, scroll_x, scroll_y);
        }

        scroll_x &= s_x_scroll_mask;
        scroll_y &= s_y_scroll_mask;

        {
        	// Render the line using the potentially modified 'scroll_y' register

        	const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
        	const std::lock_guard<std::mutex> lock_cram(mCRAMMutex);
			const std::lock_guard<std::mutex> lock_nametables(mNametablesMutex);

        	// fill scanline
        	#pragma unroll
        	for(uint16_t x = 0; x < FBDIMS.width; ++x) {
        		pHostLineData[x] = mPalette.getColor((x + mFrameNumber) / 8);
        	}
        }
    }

    // blit interal framebuffer to host (e.g Renderer) framebuffer
	//blitNativeFramebufferToHost<FBDIMS>(pNativeFrameData, reinterpret_cast<uint32_t*>(pFrameData), mPalette.data());

	mFrameNumber++;
	return true;
}

// Fast unsafe using static array with memcopy
template <uint16_t WIDTH, uint16_t HEIGHT>
inline void fillRectM(uint8_t* pFrameData, uint32_t stride_bytes, uint16_t x, uint16_t y, uint32_t color) {
    static std::array<uint32_t, WIDTH> tmp_row;
    static uint32_t last_color = color;

    if(last_color != color) {
		#pragma unroll
    	for (size_t i = 0; i < static_cast<size_t>(WIDTH); ++i) {
        	tmp_row[i] = color;
    	}
    	last_color = color;
	}

    #pragma unroll
    for(uint16_t line = 0; line < HEIGHT; ++ line) {
    	std::memcpy(pFrameData + x*4 + (line + y) * stride_bytes, tmp_row.data(), WIDTH * 4);
	}
}

// Fast unsafe using std::fill_n
template <uint16_t WIDTH, uint16_t HEIGHT>
inline void fillRectN(uint8_t* pFrameData, uint32_t stride_bytes, uint16_t x, uint16_t y, uint32_t color) {
    for (uint16_t line = 0; line < HEIGHT; ++line) {
        uint32_t* pRowPixel = reinterpret_cast<uint32_t*>(pFrameData + (line + y) * stride_bytes) + x;
        std::fill_n(pRowPixel, WIDTH, color);
    }
}

template <FramebufferDims FBDIMS>
void NesPPU<FBDIMS>::renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) {

	// palette
	{
		static const uint16_t palette_entry_width = 8;
		static const uint16_t palette_entry_height = 8;
		static const uint16_t palette_cols_count = 16;
		static const uint16_t palette_rows_count = 4;
		static const uint16_t palette_spacing = 1;
		static constexpr uint16_t palette_width = (palette_cols_count * palette_entry_width) + ((palette_cols_count - 1 ) * palette_spacing);
		static constexpr uint16_t palette_height = (palette_rows_count * palette_entry_height) + ((palette_rows_count - 1 ) * palette_spacing);

		static_assert(palette_cols_count * palette_rows_count == 64);
		static_assert(palette_width <= FBDIMS.width);
		static_assert(palette_height <= FBDIMS.height);

		
		uint16_t palette_start_y = FBDIMS.height - palette_height;

		#pragma unroll
		for (uint16_t y = 0; y < palette_rows_count; ++y) {
		
			uint16_t palette_start_x = FBDIMS.width - palette_width;

			#pragma unroll
			for (uint16_t x = 0; x < palette_cols_count; ++x) {
				uint32_t color = mPalette.getColor((x +y * palette_cols_count) & 0x3F);
				fillRectM<palette_entry_width, palette_entry_height>(pFrameData, stride_bytes, palette_start_x, palette_start_y, color);

				palette_start_x += palette_entry_width + palette_spacing;
			}

			palette_start_y += palette_entry_height + palette_spacing;
	    }
	}
}

template class CRAM<NesPPU_BASE::CRAMEntryType, 4, 2>;

template class Abstract_PPU<Platform::NES>;
template class NesPPU<{512, 288}>;

}  // namespace PPU

}  // namespace RetroCore