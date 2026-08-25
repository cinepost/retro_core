#include "framework/ppu/ppu_msx.h"
#include "framework/ppu/ppu_utils.h"
#include "../assets/msx/knightmare_ref.h"

#include "lodepng/lodepng.h"


namespace RetroCore {

namespace PPU {

static_assert(sizeof(MsxPPU_BASE::Sprite) == 8); 

static inline bool read_bit(std::uint8_t byte, uint8_t position) {
    return (byte >> position) & 1;
}

static inline bool read_bit(std::uint16_t word, uint8_t position) {
    return (word >> position) & 1;
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
	return true;
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::deinit() {
	return true;
}

void MsxPPU_BASE::setDefaultPalette() {
	mPalette = Palettes::kMsxDefaultPalette;

	// quantize to real v9938 palette
	for(uint8_t i = 0; i < 16; ++i) {
		mPalette[i] = v9938_to_rgb888(rgb888_to_v9938(mPalette[i]));
	}
}

void MsxPPU_BASE::setScreenMode(ScreenMode mode) {
	const std::lock_guard<std::mutex> lock_frame(mFrameMutex);
	mScreenMode = mode;

	switch(mScreenMode) {
		case MsxPPU_BASE::ScreenMode::VSCREEN_1:
		case MsxPPU_BASE::ScreenMode::VSCREEN_2:
			mSpriteMode = MsxPPU_BASE::SpriteMode::MODE1;
			setDefaultPalette();
			break;
		case MsxPPU_BASE::ScreenMode::VSCREEN_5:
			mSpriteMode = MsxPPU_BASE::SpriteMode::MODE1;
		default:
			break;
	}

	mScanlineCallback = nullptr;
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::loadIndexedImagePNG(const std::string& filename, uint32_t vram_address, uint16_t img_width, uint16_t img_height, Palette<16>* pPalette) {
	// TODO: check VRAM boundaries
	std::vector<uint8_t> tmp;
	if(!Utils::loadIndexedPng<16>(filename, img_width, img_height, tmp, pPalette)) {
		return false;
	}

	for(size_t i = 0; i < img_height; ++i) {
		const uint8_t* pSrc = tmp.data() + i * img_width;
		uint8_t* pDst = &mVRAM[vram_address + (i * FBDIMS.width)];
		std::memcpy(pDst, pSrc, img_width);
	}
	return true;
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::cmdHMMM(uint16_t source_x, uint16_t source_y, uint16_t dest_x, uint16_t dest_y, uint16_t block_width, uint16_t block_height, bool TMDbit /* index 0 color key */) {
	if(block_width == 0 || block_height == 0) return;

	switch(mScreenMode) {
		case MsxPPU_BASE::ScreenMode::VSCREEN_5:
		case MsxPPU_BASE::ScreenMode::VSCREEN_7:
			{
				const uint16_t screen_line_stride = FBDIMS.width;
				if(!TMDbit) {
					const uint16_t block_stride = block_width;
					for(size_t i = 0; i < block_height; ++i) {
						const uint8_t* pSrc = &mVRAM[(source_x) + (screen_line_stride * source_y++)];
						uint8_t* pDst = &mVRAM[(dest_x) + (screen_line_stride * dest_y++)];
						std::memcpy(pDst, pSrc, block_stride);
					}
				} else {
					const uint16_t block_stride = block_width;
					for(size_t i = 0; i < block_height; ++i) {
						const uint8_t* pSrcLine = &mVRAM[(source_x) + (screen_line_stride * source_y++)];
						for(uint16_t x = 0; x < block_width; ++x) {
							const uint8_t pixel = *(pSrcLine + x);
							if(pixel & 0xFF) {
								mVRAM[(dest_x + x) + (screen_line_stride * (dest_y+i))] = pixel;
							}
						}
					}
				}
			}
			break;
		default:
			assert(false && "Unsupported screen mode");
			break;
	}
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::pushTile(uint16_t tile_index, const std::array<uint8_t, 16>& fullData) {
 	pushTile(tile_index, PATTERN_8D_8C(fullData));
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::pushTile(uint16_t tile_index, const PATTERN_8D_8C& pattern) {
	if(mScreenMode != MsxPPU_BASE::ScreenMode::VSCREEN_1 && mScreenMode != MsxPPU_BASE::ScreenMode::VSCREEN_2 && mScreenMode != MsxPPU_BASE::ScreenMode::VSCREEN_4) {
		std::cerr << "Warnging! PATTERN_8D_8C data not supported in " << to_string(mScreenMode);
		return;
	}

	tile_index = tile_index % kMaximumPatternsCount; // wrap index around
	const uint32_t tile_address_offset = tile_index * 8;

	std::memcpy(&mVRAM[getPatternTableAddress() + tile_address_offset], pattern.tile.data(), 8);
	std::memcpy(&mVRAM[getColorTableAddress() + tile_address_offset], pattern.color.data(), 8);
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::pushSpritePattern(uint16_t tile_index, const uint8_t* pSrc, uint8_t bytes_count) {
	assert(bytes_count == 8 || bytes_count == 32);
	uint8_t* pDst = &mVRAM[getSpritePatternTableAddress() + (tile_index << 3)];
	std::memcpy(pDst, pSrc, bytes_count);
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::pushSpritePattern(uint16_t tile_index, const std::array<uint8_t, 8>& src) {
	pushSpritePattern(tile_index, src.data(), 8);
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::pushSpritePattern(uint16_t tile_index, const std::array<uint8_t, 32>& src) {
	pushSpritePattern(tile_index, src.data(), 32);
}

void MsxPPU_BASE::writeTileIndex(uint16_t name_table_offset, uint16_t tile_index) {
	uint16_t* pNameTable = reinterpret_cast<uint16_t*>(&mVRAM[getNameTableAddress()]);
	pNameTable[name_table_offset] = tile_index;
}

static inline void drawSpritesLine() {

}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::render_SCREEN_2(uint8_t* pFrameData, uint32_t stride_bytes) {
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
			const uint16_t pattern_index = pNameTable[x >> 3];
			const uint8_t pattern_line = current_line & 7;
			const uint8_t pattern_byte = pPatternData[(pattern_index << 3) + pattern_line];
			const uint8_t pattern_line_colors = pColorData[(pattern_index << 3) + pattern_line];
			
			uint8_t pattern_bit = read_bit(pattern_byte, 7 - x & 7);
//			const uint8_t clr = pattern_bit == 0 ? mPalette[pattern_line_colors >> 4] : mPalette[pattern_line_colors & 0x0F];

			const uint8_t clr = pattern_bit == 0 ? (pattern_line_colors & 0x0F) : (pattern_line_colors >> 4);


			*(pDst++) = ((clr == 0) ? mPalette[getBorderBackgroundColor()] : mPalette[clr]) >> 8; // RGBA8888 to XRGB8888
		}

		// Render sprites
		render_SPRITES_LINE<ScreenMode::VSCREEN_2>(line, pFrameData, stride_bytes);
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

		// Render sprites
		render_SPRITES_LINE<ScreenMode::VSCREEN_5>(line, pFrameData, stride_bytes);
	}

	return true;
}

template <FramebufferDims FBDIMS>
void MsxPPU<FBDIMS>::outputBlankScreen(uint8_t*pFrameData, uint32_t stride_bytes) {
	static std::array<uint32_t, FBDIMS.width> s_line_buffer;

	uint32_t color = 0x00000000;
	switch(mScreenMode) {
		case MsxPPU_BASE::ScreenMode::VSCREEN_8:
			color = getBorderBackgroundColor(); // GRB332
			break;
		case MsxPPU_BASE::ScreenMode::VSCREEN_2:
		case MsxPPU_BASE::ScreenMode::VSCREEN_5:
			color = mPalette[getBorderBackgroundColor()]; 
			break;
		default:
			assert(false && "Should not be here");
			break;
	}

	for(uint16_t x = 0; x < FBDIMS.width; ++x) {
		s_line_buffer[x] = color;
	}

	for(uint16_t line = 0; line < FBDIMS.height; ++line) {
		std::memcpy(pFrameData + line*stride_bytes, s_line_buffer.data(), stride_bytes);
	}
}

template <FramebufferDims FBDIMS>
bool MsxPPU<FBDIMS>::render(uint8_t* pFrameData, uint32_t stride_bytes) {
	static_assert(FBDIMS.height > 0);
	const std::lock_guard<std::mutex> lock_frame(mFrameMutex);

	mCollisionDetected = false;

	if(!mBlankingFlag) {
		outputBlankScreen(pFrameData, stride_bytes);
		return false;
	}

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