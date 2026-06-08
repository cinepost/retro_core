#include "framework/ppu/ppu_msx.h"
#include "framework/ppu/ppu_utils.h"
#include "../assets/msx/knightmare_ref.h"

#include "lodepng/lodepng.h"


namespace RetroCore {

namespace PPU {

static inline bool read_bit(std::uint8_t byte, uint8_t position) {
    return (byte >> position) & 1;
}

template<typename T>
void MsxPPU_BASE::vramBlockSet(uint32_t vram_address, const T& value, uint16_t count) {
	for(uint16_t i = 0; i < count; ++i) {
		std::memcpy(&mVRAM[vram_address + sizeof(T) * i], reinterpret_cast<const uint8_t*>(&value), sizeof(T));
	}
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::init() {
	std::memset(mVRAM.data(), 0, mVRAM.size() * sizeof(uint8_t));
	mSpritesDisableFlag = true; // It's not canonical, but we disable sprites upon initialization to make out life easier.
	return true;
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::deinit() {
	return true;
}

void MsxPPU_BASE::setScreenMode(ScreenMode mode) {
	const std::lock_guard<std::mutex> lock_frame(mFrameMutex);
	mScreenMode = mode;

	switch(mScreenMode) {
		case MsxPPU_BASE::ScreenMode::VSCREEN_1:
		case MsxPPU_BASE::ScreenMode::VSCREEN_2:
			mPalette = Palettes::kMsxDefaultPalette;
			break;
		default:
			break;
	}
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::loadIndexedImagePNG(const std::string& filename, uint32_t vram_address, uint16_t width, uint16_t height, Palette<16>& img_palette) {
	// TODO: check VRAM boundaries
	return Utils::loadIndexedPng<16>(filename, width, height, &mVRAM[vram_address], img_palette);
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::sortSprites() {
	mVisibleSpritesCount = 0;

	if(mSpritesDisableFlag) return;

	const uint8_t sprite_extent = getCurrentSpritesExtent();
	const Sprite* pSprites = reinterpret_cast<const Sprite*>(&mVRAM[getSpriteAttributeTableAddress()]);

	UNROLL_64
	for(uint16_t i = 0; i < kMaximumSpritesCount; ++i) {
		const Sprite& sprite = pSprites[i];
		if( (sprite.x >= FBDIMS.width) || (sprite.y >= FBDIMS.height) ||
		   ((sprite.x + sprite_extent) < 0) || ((sprite.y + sprite_extent) < 0)) continue;

		mVisibleSpriteIndices[mVisibleSpritesCount] = i;
		mVisibleSpritesCount++;
	}   	
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::pushTile(uint16_t tile_index, const PATTERN_8D_8C& pattern) {
	assert(mScreenMode == MsxPPU_BASE::ScreenMode::VSCREEN_2);
	if(mScreenMode != MsxPPU_BASE::ScreenMode::VSCREEN_2) {
		std::cerr << "Warnging! PATTERN_8D_8C data not supported in " << to_string(mScreenMode);
	}

	tile_index = tile_index % kMaximumPatternsCount; // wrap index around
	const uint32_t tile_address_offset = tile_index * 8;

	std::memcpy(&mVRAM[getPatternTableAddress() + tile_address_offset], pattern.tile.data(), 8);
	std::memcpy(&mVRAM[getColorTableAddress() + tile_address_offset], pattern.color.data(), 8);
}

void MsxPPU_BASE::writeTileIndex(uint16_t name_table_offset, uint16_t tile_index) {
	uint16_t* pNameTable = reinterpret_cast<uint16_t*>(&mVRAM[getNameTableAddress()]);
	pNameTable[name_table_offset] = tile_index;
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::render_SCREEN_2(uint8_t* pFrameData, uint32_t stride_bytes) {
	assert(1 == 2 && "SCREEN_2 Unimplemented!");

	static constexpr uint16_t s_y_scroll_mask = FBDIMS.height * 2;
	static constexpr uint32_t name_table_entry_size = sizeof(uint16_t); // 16 bit indices.
	static constexpr uint16_t tiles_per_line = FBDIMS.width / 8;
	static constexpr uint16_t nametable_bytes_per_line = tiles_per_line * name_table_entry_size;

	uint32_t* pDst = reinterpret_cast<uint32_t*>(pFrameData);

	#pragma unroll
	for(uint16_t line = 0; line < FBDIMS.height; ++line) {
		if (mScanlineCallback != nullptr) {
            mScanlineCallback(line);
        }

        uint16_t current_line = (line + getScrollY()) % s_y_scroll_mask;

		const uint32_t name_table_address = getNameTableAddress();
		const uint8_t* pPatternData = &mVRAM[getPatternTableAddress()];
		const uint8_t* pColorData = &mVRAM[getColorTableAddress()];

		const uint16_t* pNameTable = reinterpret_cast<const uint16_t*>(&mVRAM[getNameTableAddress() + (current_line >> 3) * nametable_bytes_per_line]);

		#pragma unroll
		for(uint16_t x = 0; x < FBDIMS.width; ++x) {
			const uint32_t pattern_index = static_cast<uint32_t>(*(pNameTable + (x >> 3)));
			const uint8_t pattern_line = current_line & 7;
			const uint8_t pattern_byte = pPatternData[(pattern_index << 3) + pattern_line];
			const uint8_t pattern_line_colors = pColorData[(pattern_index << 3) + pattern_line];
			
			uint8_t pattern_bit = read_bit(pattern_byte, 7 - x & 7);
			const uint8_t clr = pattern_bit == 0 ? mPalette[pattern_line_colors >> 4] : mPalette[pattern_line_colors & 0x0F];

			*(pDst++) = ((clr == 0) ? mPalette[getBorderBackgroundColor()] : mPalette[clr]) >> 8; // RGBA8888 to XRGB8888
		}
	}

	return true;
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::render_SCREEN_5(uint8_t* pFrameData, uint32_t stride_bytes) {

	static constexpr uint16_t s_x_scroll_mask = FBDIMS.width * 2;
	static constexpr uint16_t s_y_scroll_mask = FBDIMS.height * 2;
	static constexpr uint32_t s_page_size = FBDIMS.width * FBDIMS.height;

	uint32_t* pDst = reinterpret_cast<uint32_t*>(pFrameData);

	#pragma unroll
	for(uint16_t line = 0; line < FBDIMS.height; ++line) {

		if (mScanlineCallback != nullptr) {
            mScanlineCallback(line);
        }

        const uint8_t* pVRAMPage = getVramPagePtr(getCurrentVramPageIndex());
		uint16_t current_line = (line + getScrollY()) % s_y_scroll_mask;

		#pragma unroll
		for(uint16_t x = 0; x < FBDIMS.width; ++x) {
			uint32_t current_x = (x + getScrollX()) % s_x_scroll_mask;

			if(current_x >= FBDIMS.width) {
				current_x = current_x % FBDIMS.width;
				current_line += FBDIMS.height;
				current_line = current_line % FBDIMS.height;
			}

			const uint8_t clr = pVRAMPage[current_line * FBDIMS.width + current_x];

			*(pDst++) = ((clr == 0) ? mPalette[getBorderBackgroundColor()] : mPalette[clr]) >> 8; // RGBA8888 to XRGB8888
		}

		if(mSpritesDisableFlag || mVisibleSpritesCount == 0) {
			// Sprites disabled. 
			continue;
		}

		// Render sprites
	}

	return true;
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::render(uint8_t* pFrameData, uint32_t stride_bytes) {
	static_assert(FBDIMS.height > 0);
	const std::lock_guard<std::mutex> lock_frame(mFrameMutex);

	mCollisionDetected = false;

	bool result = false;

	sortSprites();

	switch(mScreenMode) {
		case MsxPPU_BASE::ScreenMode::VSCREEN_2:
			result = render_SCREEN_2(pFrameData, stride_bytes);
			break;
		case MsxPPU_BASE::ScreenMode::VSCREEN_5:
			result = render_SCREEN_5(pFrameData, stride_bytes);
			break;
		default:
			break;
	}

	renderDebugScreen(pFrameData, stride_bytes);

	mFrameNumber++;
	return result;
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
void MsxPPU<FBDIMS>::renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) {
	
	// Display test knighmare image
	if(1 == 2){
		static constexpr uint16_t top 	= (FBDIMS.height / 2) - (192 / 2);
		static constexpr uint16_t left 	= (FBDIMS.width  / 2) - (256 / 2);
		static const uint16_t im_width = 256;
		static const uint16_t im_height = 192 / 2;
		
		const uint8_t* pSrc = RetroCore::StaticData::MSX::gTestImage.pixel_data;

		#pragma unroll
		for(uint16_t line = 0; line < im_height; ++line) {
			uint8_t* pTarget = pFrameData + left * 4 + (top + line) * stride_bytes;
			uint32_t t;
			std::memcpy(pTarget, pSrc, im_width * 4);
			pSrc += im_width * 4;
		} 
	}

	// palette
	{
		static const uint16_t palette_entry_width = 8;
		static const uint16_t palette_entry_height = 8;
		static const uint16_t palette_cols_count = 16;
		static const uint16_t palette_rows_count = 1;
		static const uint16_t palette_spacing = 1;
		static constexpr uint16_t palette_width = (palette_cols_count * palette_entry_width) + ((palette_cols_count - 1 ) * palette_spacing);
		static constexpr uint16_t palette_height = (palette_rows_count * palette_entry_height) + ((palette_rows_count - 1 ) * palette_spacing);

		static_assert(palette_cols_count * palette_rows_count == 16);
		static_assert(palette_width <= FBDIMS.width);
		static_assert(palette_height <= FBDIMS.height);

		
		uint16_t palette_start_y = FBDIMS.height - palette_height;

		#pragma unroll
		for (uint16_t y = 0; y < palette_rows_count; ++y) {
		
			uint16_t palette_start_x = FBDIMS.width - palette_width;

			#pragma unroll
			for (uint16_t x = 0; x < palette_cols_count; ++x) {
				uint32_t color = mPalette.getColor((x + y * palette_cols_count) & 0x0F) >> 8; // RGBA8888 to XRGB8888
				fillRectM<palette_entry_width, palette_entry_height>(pFrameData, stride_bytes, palette_start_x, palette_start_y, color);

				palette_start_x += palette_entry_width + palette_spacing;
			}

			palette_start_y += palette_entry_height + palette_spacing;
	    }
	}
}

// Specialization

template class Abstract_PPU<Platform::MSX>;

template class MsxPPU<{512, 288}>;

template void MsxPPU_BASE::vramBlockSet<uint8_t>(uint32_t vram_address, const uint8_t& value, uint16_t count);
template void MsxPPU_BASE::vramBlockSet<uint16_t>(uint32_t vram_address, const uint16_t& value, uint16_t count);
template void MsxPPU_BASE::vramBlockSet<uint32_t>(uint32_t vram_address, const uint32_t& value, uint16_t count);
template void MsxPPU_BASE::vramBlockSet<MsxPPU_BASE::Sprite>(uint32_t vram_address, const MsxPPU_BASE::Sprite& value, uint16_t count);

}  // namespace PPU

}  // namespace RetroCore