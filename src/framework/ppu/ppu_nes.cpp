#include "framework/ppu/ppu_nes.h"

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

		uint16_t scroll_y = mScrollY + scanline;
		uint16_t scroll_x = mScrollX;
		        
        // Per-scanline callback if registered one
        if (mScanlineCallback != nullptr) {
            mScanlineCallback(scanline, scroll_x, scroll_y);
        }

        static constexpr uint16_t s_x_scroll_mask = getIndexMask<uint16_t, uint16_t>(FBDIMS.width);
        static constexpr uint16_t s_y_scroll_mask = getIndexMask<uint16_t, uint16_t>(FBDIMS.height);

        scroll_x &= s_x_scroll_mask;
        scroll_y &= s_y_scroll_mask;

        {
        	// Render the line using the potentially modified 'scroll_y' register

        	const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
        	const std::lock_guard<std::mutex> lock_cram(mCRAMMutex);
			const std::lock_guard<std::mutex> lock_nametables(mNametablesMutex);

        	uint8_t* pLineData = pFrameData + scanline * stride_bytes;
        	pLineData[100] = 255;
        	pLineData[101] = 255;
        	pLineData[102] = 255;
        	//pLineData[403] = 255;
        }
    }
	
	mFrameNumber++;
	return true;
}

template <uint16_t WIDTH, uint16_t HEIGHT>
inline void fillRect(uint8_t* pFrameData, uint32_t stride_bytes, uint16_t x, uint16_t y, uint32_t color) {
    static std::array<uint32_t, WIDTH> tmp_row;

	#pragma unroll
    for (size_t i = 0; i < WIDTH; ++i) {
        std::memcpy(&tmp_row[i], &color, 4);
    }

    #pragma unroll
    for(uint32_t line = 0; line < HEIGHT; ++ line) {
    	std::memcpy(pFrameData + x*4 + (line + y) * stride_bytes, tmp_row.data(), WIDTH * 4);
	}
}

template <FramebufferDims FBDIMS>
void NesPPU<FBDIMS>::renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) {

	// palette
	std::array<uint32_t, FBDIMS.width> rowA;
	std::array<uint32_t, FBDIMS.width> rowB;
    
	static const uint16_t palette_box_width = 8;
	static const uint16_t palette_box_height = 8;
	static const uint16_t palette_cols_count = 16;
	static const uint16_t palette_rows_count = 4;

	static_assert(palette_cols_count * palette_rows_count == 64);
	static_assert(palette_box_width * palette_cols_count <= FBDIMS.width);
	static_assert(palette_box_height * palette_rows_count <= FBDIMS.height);
/*
    for (int x = 0; x < palette_cols_count * palette_box_width; ++x) {
        bool col_even = (x / square_size) % 2 == 0;
        rowA[x] = col_even ? color1 : color2;
        rowB[x] = col_even ? color2 : color1;
    }
*/

	#pragma unroll
	for (uint16_t y = 0; y < palette_rows_count; ++y) {
		
		#pragma unroll
		for (uint16_t x = 0; x < palette_cols_count; ++x) {
			uint32_t color;
			std::memcpy(&color, mPalette.getColor(x+y*palette_cols_count).data(), 4);
		    fillRect<palette_box_width, palette_box_height>(pFrameData, stride_bytes, x * (palette_box_width + 1), y * (palette_box_height + 1), color);
		}
    }
}

template class CRAM<NesPPU_BASE::CRAMEntryType, 4, 2>;

template class Abstract_PPU<Platform::NES>;
template class NesPPU<{512, 288}>;

}  // namespace PPU

}  // namespace RetroCore