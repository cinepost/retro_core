#ifndef __RETRO_CORE_FRAMEWORK_PPU_PPU_NES_H
#define __RETRO_CORE_FRAMEWORK_PPU_PPU_NES_H

#include "framework/static_data.h"
#include "framework/palette.h"
#include "framework/ppu/ppu.h"

#include <cstdint>
#include <vector>
#include <cassert>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>
#include <mutex>
#include <functional>


namespace RetroCore {

namespace StaticData { 
namespace NES {
    // Default font CHR-ROM bank
    extern const std::array<uint8_t, 1536> DefaultFontCHR; 

}  // namespace NES
}  // namespace StaticData

namespace PPU {

class NesPPU_BASE: public Abstract_PPU<Platform::NES> {
	public:
		static const uint16_t kMaximumSpriteIndex = 1023;
		static constexpr uint16_t kMaximumTileIndex = std::numeric_limits<uint16_t>::max();
		static constexpr uint16_t sSpriteIndexMask = getIndexMask<uint16_t, uint16_t>(kMaximumSpriteIndex);
		static constexpr uint16_t sTileIndexMask = getIndexMask<uint16_t, uint16_t>(kMaximumTileIndex);

		using CRAMEntryType = std::array<uint8_t, 4>;
		using ScanlineCallback = std::function<void(const uint16_t, uint16_t&, uint16_t&)>;

		using CHRTile = std::array<uint8_t, 16>; // 8x8 pixels 2 bits per pixel sprite data
		using PatternTable = std::array<CHRTile, std::numeric_limits<uint16_t>::max() + 1>; // 1mb of chr data

		struct alignas(8) Sprite {
			static constexpr uint16_t 	kInvalidTileIndex = std::numeric_limits<uint16_t>::max();
			static constexpr int16_t 	kOffScreenPos = std::numeric_limits<int16_t>::min();
			int16_t x;
			int16_t y;
			uint16_t tile_index;
			uint8_t attibs;
			uint8_t _pad;

			Sprite(): x(kOffScreenPos), y(kOffScreenPos), tile_index(kInvalidTileIndex), attibs(0) { }
			Sprite(int16_t _x, int16_t _y, uint16_t _tile_index, uint8_t _attibs): x(_x), y(_y), tile_index(_tile_index & sTileIndexMask), attibs(_attibs) { }
		};

		using OAM = std::array<Sprite, kMaximumSpriteIndex + 1>;

		void setSprite(uint16_t sprite_index, uint16_t tile_index, int16_t x, int16_t y, uint8_t attibs) {
			const std::lock_guard<std::mutex> lock_frame(mFrameMutex);
			mSprites[sprite_index & sSpriteIndexMask] = {x, y, tile_index, attibs};
		}

		void setSpritesState(bool state) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mSpritesEnabled = state;
		}

		bool getSpritesState() const { return mSpritesEnabled; }

		void setBackgroundState(bool state) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mBackgroundEnabled = state;
		}

		bool getBackgroundState() const { return mBackgroundEnabled; }

		// Set per-scanline callback
    	void setScanlineCallback(ScanlineCallback cb) {
        	mScanlineCallback = cb;
    	}

    	void updateCRAM(uint8_t line_index, uint8_t subpalette_index, const std::array<uint8_t, 4>& values) { 
			const std::lock_guard<std::mutex> lock_cram(mCRAMMutex);
			mCRAM.getCRAMLine(line_index)[subpalette_index & 0x03] = values;
		}

		void updateCRAM(uint8_t line_index, uint8_t subpalette_index, uint8_t color_index, uint8_t color) { 
			const std::lock_guard<std::mutex> lock_cram(mCRAMMutex);
			mCRAM.getCRAMLine(line_index)[subpalette_index & 0x03][color_index & 0x03] = color;
		}

		void setSpritesPatternTableID(uint8_t index) {
			const std::lock_guard<std::mutex> lock_nametables(mScanlineMutex);
			mSpritesPatternTableID = index & 0x01;
		}

		void setBackgroundPatternTableID(uint8_t index) {
			const std::lock_guard<std::mutex> lock_nametables(mScanlineMutex);
			mBackgroundPatternTableID = index & 0x01;
		}

		template<uint8_t patternTableId>
		void pushTiles(const uint8_t* pData, size_t count, uint16_t tileIndex = 0 /* push starting at tileIndex */) {
			static constexpr size_t tileSizeBytes = sizeof(CHRTile);
			uint8_t* pTarget; 
			if constexpr (patternTableId == 0) {
				pTarget = mPatternTable0[tileIndex].data();
			} else {
				pTarget = mPatternTable1[tileIndex].data();
			}

			std::memcpy(pTarget, pData, count);
		}

		NesPPU_BASE(): Abstract_PPU<Platform::NES>() {

		}

	public:
		static const uint8_t* getDefaultFontData(size_t& bytes_count) {
			bytes_count = StaticData::NES::DefaultFontCHR.size();
			return StaticData::NES::DefaultFontCHR.data();
		}

	protected:
		static constexpr Palette<64> mPalette = Palettes::kNesDefaultPalette;
		ScanlineCallback mScanlineCallback = nullptr;

		// Internal PPU state and registers
		uint16_t 	mScrollX = 0; // Equivalent to register $2005
		uint16_t 	mScrollY = 0; // Equivalent to register $2005

		uint8_t  	mSpritesPatternTableID = 0; 	// Equivalent to bit 3 of register $2000
		uint8_t  	mBackgroundPatternTableID = 1; 	// Equivalent to bit 4 of register $2000
		bool 		mSpritesEnabled = false; 		// Equivalent to bit 4 of register $2001
		bool        mBackgroundEnabled = false;     // Equivalent to bit 3 of register $2001

		std::array<Sprite*, kMaximumSpriteIndex + 1> mVisibleSprites;
    	size_t mVisibleSpritesCount = 0;

    	size_t mFrameNumber = 0;


		// PPU ram
		OAM        					mSprites;
		PatternTable 				mPatternTable0;
		PatternTable 				mPatternTable1;
		CRAM<CRAMEntryType, 4, 2> 	mCRAM;

		// 
		std::mutex      mCRAMMutex;
		std::mutex      mNametablesMutex;
		
		std::mutex      mFrameMutex;
		std::mutex      mScanlineMutex;
};

template <FramebufferDims FBDIMS>
class NesPPU: public NesPPU_BASE {
	public:
		static constexpr uint16_t s_x_scroll_mask = isPowerOfTwo(FBDIMS.width) ? FBDIMS.width - 1 : std::numeric_limits<uint16_t>::max();
		static constexpr uint16_t s_y_scroll_mask = isPowerOfTwo(FBDIMS.height) ? FBDIMS.height - 1 : std::numeric_limits<uint16_t>::max();
        static constexpr size_t s_native_fb_stride_bytes = FBDIMS.width;

		struct Nametable {
			// Tile table constants
			static constexpr uint16_t TILE_TBL_WIDTH = divideExact<FBDIMS.height, 8>();
			static constexpr uint16_t TILE_TBL_HEIGHT = divideExact<FBDIMS.width, 8>();
			
			// Attribute table constants. 32x32 pixel coverage 2bit subpalette intex per 8x8 tile
			static constexpr uint16_t ATTR_TBL_WIDTH = TILE_TBL_WIDTH / 4;
			static constexpr uint16_t ATTR_TBL_HEIGHT = TILE_TBL_HEIGHT / 4;

			static_assert((uint32_t)TILE_TBL_WIDTH * (uint32_t)TILE_TBL_HEIGHT <= (std::numeric_limits<uint16_t>::max() + 1));
			static_assert(ATTR_TBL_WIDTH * 4 == TILE_TBL_WIDTH);
			static_assert(ATTR_TBL_HEIGHT * 4 == TILE_TBL_HEIGHT);

			using TileID = uint16_t;
			using Attrib = uint8_t;
			std::array<TileID, TILE_TBL_WIDTH * TILE_TBL_HEIGHT> tileTable;
			std::array<Attrib, ATTR_TBL_WIDTH * ATTR_TBL_HEIGHT> attrTable;
		};

		NesPPU(): NesPPU_BASE() {
			assert(mNativeFramebuffer.size() == FBDIMS.width * FBDIMS.height);
		}

	public:
		bool init();
		bool deinit();
		bool render(uint8_t* pFrameData, uint32_t stride_bytes);
		void renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) override;

	public:
		void setScroll(uint16_t x, uint16_t y) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);

			mScrollX = x & s_x_scroll_mask;
			mScrollY = y & s_y_scroll_mask;
		}

		void setNametableTileID(uint8_t table_x, uint8_t table_y, uint16_t tile_x, uint16_t tile_y, Nametable::TileID tile_id) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mNametables[table_y & 0x01][table_x & 0x01].tileTable[tile_x + tile_y] = tile_id;
		}

		void setNametableAttrib(uint8_t table_x, uint8_t table_y, uint16_t attr_x, uint16_t attr_y, uint8_t attrib) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mNametables[table_y & 0x01][table_x & 0x01].attrTable[attr_x + attr_y] = attrib;
		}

	private:
		static constexpr FramebufferDims 	mFramebufferSize = FBDIMS;
		static constexpr uint16_t    		mNativeFramebufferStride = nativeFramebufferStride(Platform::NES, FBDIMS.width);

	private:
		// Internal PPU state and registers
		Nametable 	mNametables[2][2]; // 4 screen nametables

		alignas(64) std::array<uint8_t, nativeFramebufferSize(Platform::NES, FBDIMS.width, FBDIMS.height)> mNativeFramebuffer;
};

}  // namespace PPU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_NES_H