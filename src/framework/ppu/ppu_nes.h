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
		static const uint16_t kMaximumSprites = 1024;
		using CRAMEntryType = std::array<uint8_t, 4>;
		using ScanlineCallback = std::function<void(const uint16_t, uint16_t&, uint16_t&)>;

		using CHRTile = std::array<uint8_t, 16>; // 8x8 pixels 2 bits per pixel sprite data
		using PatternTable = std::array<CHRTile, std::numeric_limits<uint16_t>::max() + 1>; // 1mb of chr data

		struct Sprite {
			static constexpr uint16_t kInvalidTileIndex = std::numeric_limits<uint16_t>::max();
			int16_t x;
			int16_t y;
			uint16_t tile_index;
			uint16_t attibs;

			Sprite(): x(0), y(0), tile_index(kInvalidTileIndex), attibs(0) { }

		};

		using OAM = std::array<Sprite, kMaximumSprites>;

		NesPPU_BASE(): Abstract_PPU<Platform::NES>() {

		}

	public:
		static const uint8_t* getDefaultFontData(size_t& bytes_count) {
			bytes_count = StaticData::NES::DefaultFontCHR.size();
			return StaticData::NES::DefaultFontCHR.data();
		}
};

template <FramebufferDims FBDIMS>
class NesPPU: public NesPPU_BASE {
	public:
		struct Nametable {
			struct TileID {
				uint8_t patternId;
				uint8_t patternTableId;
			};

			using Attribute = uint8_t;
			std::array<std::array<TileID, divideExact<FBDIMS.height, 8>()>, divideExact<FBDIMS.width, 8>()> tileMap; // ROW major nambetable
			std::array<std::array<Attribute, divideExact<FBDIMS.height, 16>()>, divideExact<FBDIMS.width, 16>()> attributeTable; // ROW major nambetable
		};
		using Nametables = std::array<std::array<Nametable, 2>, 2>;

		NesPPU(): NesPPU_BASE() {

		}

	public:
		bool init() override;
		bool deinit() override;
		bool render(uint8_t* pFrameData, uint32_t stride_bytes) override;

	public:
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

		void setScroll(uint16_t x, uint16_t y) {

		}

		void setSpritesPatternTableID(uint8_t index) {
			const std::lock_guard<std::mutex> lock_nametables(mScanlineMutex);
			mSpritesPatternTableID = index & 0x01;
		}

		void setBackgroundPatternTableID(uint8_t index) {
			const std::lock_guard<std::mutex> lock_nametables(mScanlineMutex);
			mBackgroundPatternTableID = index & 0x01;
		}

	private:
		static constexpr FramebufferDims 	mFramebufferSize = FBDIMS;
		static constexpr uint16_t    		mNativeFramebufferStride = nativeFramebufferStride(Platform::NES, FBDIMS.width);

	private:
		ScanlineCallback mScanlineCallback = nullptr;

		// Internal PPU state registers
		uint16_t mScrollX = 0;
		uint16_t mScrollY = 0;

		uint8_t  mSpritesPatternTableID = 0; 	// Equivalent to bit 3 of register $2000
		uint8_t  mBackgroundPatternTableID = 1; // Equivalent to bit 4 of register $2000

		OAM        mSprites;    
		Nametables mNametables; // 4 screen nametables

		static constexpr Palette<64> mPalette = Palettes::kNesDefaultPalette;
		CRAM<CRAMEntryType, 4, 2> 	mCRAM;
		std::array<uint8_t, nativeFramebufferSize(Platform::NES, FBDIMS.width, FBDIMS.height)> mNativeFramebuffer;

		PatternTable 	mPatternTable0;
		PatternTable 	mPatternTable1;

		// 
		std::mutex      mCRAMMutex;
		std::mutex      mNametablesMutex;
		std::mutex      mScanlineMutex;
};

}  // namespace PPU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_NES_H