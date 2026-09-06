#ifndef __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_H
#define __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_H

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
#include <queue>


// https://www.spriters-resource.com/msx/knightmare/asset/115323/

namespace RetroCore {

namespace StaticData { 
namespace MSX {

// Some static MSX PPU related data here..

}  // namespace MSX
}  // namespace StaticData

namespace PPU {

class MsxPPU_BASE: public Abstract_PPU<Platform::MSX> {
	public:
		struct PATTERN_8D_8C {
			std::array<uint8_t, 8> tile;   // 8 bytes of 8x8 1-bit tile data
    		std::array<uint8_t, 8> color;  // 8 bytes of 4-bit per line color data

    		PATTERN_8D_8C() {
    			tile.fill(0); color.fill(0);
    		}

    		// Initialize using 16 uint8_t values (8 tile, 8 color)
			PATTERN_8D_8C(const std::array<uint8_t, 16>& fullData) {
				for (size_t i = 0; i < 8; ++i) {
					tile[i]  = fullData[i];
					color[i] = fullData[i + 8];
				}
			}
	
			// 8 values for tile data and  bytes of colors
    		PATTERN_8D_8C(const std::array<uint8_t, 8>& tileData, const std::array<uint8_t, 8>& colorData) {
				tile = tileData;
				color = colorData;
			}

			// 8 values for tile data and one uint8_t color for all lines
    		PATTERN_8D_8C(const std::array<uint8_t, 8>& tileData, uint8_t singleColor) {
				tile = tileData;
				color.fill(singleColor);
			}
			
			// One uint8_t tile data for all lines and 8 bytes of colors
			PATTERN_8D_8C(uint8_t singleTile, const std::array<uint8_t, 8>& colorData) {
				tile.fill(singleTile);
				color = colorData;
			}
			
			// One uint8_t tile data for one uint8_t color for all lines
			PATTERN_8D_8C(uint8_t singleTile, uint8_t singleColor) {
				tile.fill(singleTile);
				color.fill(singleColor);
			}

			void setColor(uint8_t singleColor) {
				color.fill(singleColor);
			}

			void setColors(const std::array<uint8_t, 8>& colorData) {
				color = colorData;
			}

			/**
			* @brief Extracts the two 4-bit color nibbles for a specific line.
			* @param line The line number (0 to 7).
			* @return A pair containing <high_nibble, low_nibble>.
			*/
			[[nodiscard]] std::pair<uint8_t, uint8_t> getColorNibbles(uint8_t line) const {
				// Fallback or safety check if index is out of bounds
				
				uint8_t rawColor = color[line & 0x03];
				uint8_t high = (rawColor >> 4) & 0x0F;
				uint8_t low  = rawColor & 0x0F;

				return {high, low};
			}	
		};

		using NameTableEntry = uint16_t;

		static const uint16_t kMaximumSpritesCount = 1024;
		static const uint16_t kMaximumSpriteIndex = kMaximumSpritesCount - 1;

		static constexpr uint16_t kMaximumPatternsCount = 4096;
		static const uint16_t kMaximumPatternIndex = kMaximumPatternsCount - 1; // approx two screens of tiles at 512x288 resolution

		static const uint32_t kVRAMSizeBytes = 1024 * 1024 * 16; // 16 Mb VRAM 

		// If sprite Y is equal to kVerticalTerminatorCode must instantly stop processing any subsequent sprites in the table.
		static constexpr int16_t kVerticalTerminatorCode = std::numeric_limits<int16_t>::min();

		using ScanlineCallback = std::function<void(const uint16_t)>;
		using CollisionDetectionCallback = std::function<void(uint16_t&, uint16_t&)>;

		enum class ScreenMode: uint8_t {
			// TMS9918 equivalents
			VSCREEN_0 = 0,  // Text mode. 2 colors out of 512 fixed palette
			VSCREEN_1,		// 16 bit tile index. 16 fixed palette colors (max 2 distinct colors per 8×8 pixel grid)
			VSCREEN_2,      // 16 bit tile index. 8 bytes of 4bit  palette color indices per 8x8px tile
			VSCREEN_3, 		// 16 colors.
			VSCREEN_4,		// 16 colors. Identical to VSCREEN_2 layout, but updates sprite engine rules to SpriteMode::MODE2 (allowing multiple colors per sprite line).

			// V9938 equivalents
			VSCREEN_5,      // 16 colors indexed out of a 512-color programmable rgb333 palette.
			VSCREEN_6,      // 4 colors indexed out of a 512-color programmable rgb333 palette.
			VSCREEN_7,		// Identical to VSCREEN_5.
			VSCREEN_8,		// 256 fixed colors. 8 bits per pixel directly map to a raw GRB (3:3:2) allocation, leaving no space for software palette modifications.

			// V9958 equivalents
			VSCREEN_10,		// YJK with Palette. Up to 12,499 simultaneous colors. Combines YJK background data with regular 16-color palette data for overlays (like high-fidelity sprites).
			VSCREEN_11,		// YJK with Palette. Up to 12,499 simultaneous colors. Functions identically to VSCREEN_10 but handles the color attributes layout slightly differently for text and sprite priority.
			VSCREEN_12 		// Pure YJK. 19,268 simultaneous colors out of a 32,768 palette
		};

		enum class TextMode: uint8_t {
			NORMAL = 0, 	// Normal density text mode
			HIGH 			// High density text mode
		};

		enum class SpriteSize: uint8_t {
			SPRITE_8 = 0,	// 8x8px sprites
			SPRITE_16 		// 16x16px sprites
		};

		enum class SpriteMode: uint8_t {
			MODE1 = 0, 	// MSX1 backward compatibility mode; 4 sprites per line, 16 fixed colors
			MODE2		// MSX2 advanced mode; 8 sprites per line, multi-color lines via the Color Table
		};

		enum class AsyncCmd: uint8_t {
			NOOP = 0,
			
			// High-Speed Block Transfers (VRAM-to-VRAM)
			HMMM, // High-Speed VRAM-to-VRAM Transfer
			YMMM, // High-Speed VRAM-to-VRAM Y-only
			HMMV, // High-Speed VRAM Fill
			LMMM, // Logical VRAM-to-VRAM Transfer
			
			// CPU-Interactive Transfers
			HMMC, // High-Speed CPU-to-VRAM
			LMMC, // Logical CPU-to-VRAM
			LMCM, // Logical VRAM-to-CPU

			// Hardware Drawing Commands
			LINE, // Draw Line
			SRCH, // Search Pixel
		};

		/*  LOGOP - Logical operation
		 *
		 *	0 (Replace)
		 * 	8 (OR) 
		 *	1 (TIMK - Transparent If Move Color is King)
		 */
		using LOGOP = uint8_t;

		struct alignas(8) Sprite {
			static constexpr uint16_t 	kInvalidPatternIndex = std::numeric_limits<uint16_t>::max();
			static constexpr int16_t 	kOffScreenPos = std::numeric_limits<int16_t>::min();
			int16_t x = kOffScreenPos;	// int16_t allows sprites to be partially hidden top/left
			int16_t y = kOffScreenPos;
			uint16_t index = kInvalidPatternIndex; // Pattern index. (If using 16x16 sprites, must be a multiple of 4)
			
			struct Attributes {
				uint8_t color 	: 4; // Bits 0-3: 4 bits line color index
				uint8_t reserved: 3; // Bits 4-6: Reserved
				uint8_t ec 		: 1; // EC (Early Clock) flag. Shifts sprite 32 pixels left. // UNUSED !!!
			} attribs;
			uint8_t _pad;
		};

		struct Sprite_Color_Line {
			uint8_t color 	: 4; // Bits 0-3: 4 bits line color index
			uint8_t cc    	: 1; // Bit 4: CC (Color Code) bit for mixing or layered prioritization
			uint8_t ic    	: 1; // Bit 5: IC (Invisible Code) makes the line transparent if set
			uint8_t reserved: 1; // Bit 6: Reserved
			uint8_t priority: 1; // Bit 7: Priority flag (Over or behind backgound tiles)
		};

	public:
		MsxPPU_BASE(): Abstract_PPU<Platform::MSX>() {
			setPalette(Palettes::kMsxDefaultPalette);
			init();
		}

		virtual ~MsxPPU_BASE() = default;

	public:
		// Actual Virtual(V)(TMS)99x8 VDP API
		void setScreenMode(ScreenMode mode);

		void setSpriteMode(SpriteMode mode) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mSpriteMode = mode;
		}

		void setSpriteSize(SpriteSize size) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mSpriteSize = size;
		}

		void setBorderBackgroundColor(uint8_t color_index) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mBorderBackgroundColor = color_index;
		}

		inline uint8_t getBorderBackgroundColor() const {
			return mBorderBackgroundColor;
		}

		void enableSprites() {
			mSpritesDisableFlag = false;
		}

		void disableSprites() {
			mSpritesDisableFlag = true;
		}

		/* Equivalent og Bit 6 (BL - Blanking) of Register R1
		 *
		 * By toggling this bit, instantly turn off the visual rendering of the screen. 
		 * The VDP will stop outputting any pixel data, software/hardware sprites, or tiles, 
		 * and will instead output a solid color (either solid black or your configured backdrop border color).
		 */
		void setBlankingBit(bool state) {
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mBlankingFlag = state;
		}

		void modifyPalette(uint8_t color_index, uint16_t rgb333 /* 3 bits per channel 9 bit color value */) {
			if(!canPaletteBeModified(mScreenMode)) return;
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mPalette[color_index & 0x0F] = rgb333_to_rgba8888(rgb333);
		}

		void setDefaultPalette();

		void setPalette(const Palette<16>& palette) {
			if(!canPaletteBeModified(mScreenMode)) return;
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mPalette = palette;
			
			// quantize to real v9938 palette
			for(uint8_t i = 0; i < 16; ++i) {
				mPalette[i] = v9938_to_rgb888(rgb888_to_v9938(mPalette[i]));
			}
		}

		const Palette<16>& getPalette() const {
			return mPalette;
		}

		void writeTileIndex(uint16_t name_table_offset, uint16_t tile_index);

		[[nodiscard]] inline uint8_t getCurrentSpritesExtent() const {
			return (mSpriteSize == SpriteSize::SPRITE_8 ? 8 : 16) * (mSpritesMag ? 2 : 1);
		}

		inline void setPatternTableAddress(uint32_t address) {
			mPatternTableAddress = address % kVRAMSizeBytes;
		}

		[[nodiscard]] inline uint32_t getPatternTableAddress() const {
			return mPatternTableAddress;
		}

		inline void setColorTableAddress(uint32_t address) {
			mColorTableAddress = address % kVRAMSizeBytes;
		}

		[[nodiscard]] inline uint32_t getColorTableAddress() const {
			return mColorTableAddress;
		}

		inline void setNameTableAddress(uint32_t address) {
			mNameTableAddress = address % kVRAMSizeBytes;
		}

		[[nodiscard]] inline uint32_t getNameTableAddress() const {
			return mNameTableAddress;
		}

		inline void setSpritePatternTableAddress(uint32_t address) {
			mSpritePatternTableAddress = address % kVRAMSizeBytes;
		}

		[[nodiscard]] inline uint32_t getSpritePatternTableAddress() const {
			return mSpritePatternTableAddress;
		}

		inline void setSpriteAttributeTableAddress(uint32_t address) {
			mSpriteAttributeTableAddress = address % kVRAMSizeBytes;
		}

		[[nodiscard]] inline uint32_t getSpriteAttributeTableAddress() const {
			return mSpriteAttributeTableAddress;
		}

		inline void setSpriteColorTableAddress(uint32_t address) {
			mSpriteColorTableAddress = address % kVRAMSizeBytes;
		}

		[[nodiscard]] inline uint32_t getSpriteColorTableAddress() const {
			return mSpriteColorTableAddress;
		}

		// Per-scanline callback
    	void setScanlineCallback(ScanlineCallback cb) {
    		const std::lock_guard<std::mutex> lock_frame(mFrameMutex);
        	mScanlineCallback = cb;
    	}

		// Collision callback
    	void setCollisionDetectionCallback(CollisionDetectionCallback cb) {
    		const std::lock_guard<std::mutex> lock_frame(mFrameMutex);
        	mCollisionDetectionCallback = cb;
    	}

		// VRAM access

		void clearVRAM() {
			std::fill(mVRAM.begin(), mVRAM.end(), 0);
		}

		inline void vramWrite(uint32_t vram_address, uint8_t value) {
			mVRAM[vram_address % kVRAMSizeBytes] = value;
		}

		template<typename T>
		void vramBlockSet(uint32_t vram_address, const T& value, uint16_t count);

		inline void vramBlockWrite(uint32_t vram_address, const uint8_t* source_buffer, uint16_t num_bytes) {
			std::memcpy(&mVRAM[vram_address % kVRAMSizeBytes], source_buffer, num_bytes);
		}

		static constexpr uint32_t getPatternsTableSize() {
			return kMaximumPatternsCount * 8;
		}

		virtual uint32_t getNameTableSize() const = 0;

		static constexpr uint32_t geColorTableSize() {
			return kMaximumPatternsCount * 8;
		}

		static constexpr uint32_t getSpritePatternsTableSize() {
			return kMaximumSpritesCount * 8;
		}

		static constexpr uint32_t getSpriteAttributeTableSize() {
			return sizeof(Sprite) * kMaximumSpritesCount;
		}

		static constexpr uint32_t getSpriteColorTableSize() {
			return sizeof(Sprite_Color_Line) * kMaximumSpritesCount * 8;
		}

		// Higher level 
		void pushTile(uint16_t tile_index, const std::array<uint8_t, 16>& fullData);
		void pushTile(uint16_t tile_index, const PATTERN_8D_8C& tileData);

		void pushTiles(const std::array<uint8_t, 16>* pTiles, size_t count, uint16_t tile_index_offset) {
			assert(count <= (tile_index_offset + kMaximumPatternsCount));
			for(size_t i = 0; i < count; ++i) {
				pushTile(i + tile_index_offset, pTiles[i]);
			}
		}

		/**
 		* Pushes 8x8px 1bpp sprite pattern into VRAM.
 		*/
		void pushSpritePattern(uint16_t tile_index, const uint8_t* pSrc, uint8_t bytes_count /* 8 or 32 bytes of data */);
		void pushSpritePattern(uint16_t tile_index, const std::array<uint8_t, 8>& src);
		void pushSpritePattern(uint16_t tile_index, const std::array<uint8_t, 32>& src);

	public:
		[[nodiscard]] static constexpr bool canPaletteBeModified(ScreenMode mode) {
			switch(mode) {
				case ScreenMode::VSCREEN_8:
				case ScreenMode::VSCREEN_12:
					return false;
				default:
					return true;
			}
		}

	protected:
		CollisionDetectionCallback 	mCollisionDetectionCallback = nullptr;
		ScanlineCallback 			mScanlineCallback = nullptr;

		// Internal PPU state
		bool            mBlankingFlag       = true;
		ScreenMode 		mScreenMode         = ScreenMode::VSCREEN_5;
		TextMode        mTextMode 			= TextMode::NORMAL;
		SpriteMode      mSpriteMode 		= SpriteMode::MODE1;
		SpriteSize      mSpriteSize			= SpriteSize::SPRITE_8;	// Equivalent to Bit 1 of Control Register 1 (R1). False - 8x8, True - 16x16.
		bool            mSpritesMag 		= false;				// Equivalent to Bit 0 of Control Register 1 (R1). False - 1x scale, True - 2x scale.
		bool            mSpritesDisableFlag	= false;				// Equivalent to Bit 6 of Register 8 (R8). True - entire sprite rendering pipeline is bypassed.

		bool            mCollisionDetected 	= false;
		uint8_t         mCurrentPageIndex = 0;
		uint16_t        mScrollY = 0;
		uint16_t        mScrollX = 0;

		uint8_t         mBorderBackgroundColor = 0;

		uint32_t        mPatternTableAddress = 0;					// VRAM address multiplied by 0x800 (2048)
		uint32_t        mColorTableAddress = 0;
		uint32_t        mNameTableAddress = 0;
		uint32_t		mSpritePatternTableAddress = 0;
		uint32_t        mSpriteAttributeTableAddress = 0;
		uint32_t        mSpriteColorTableAddress = 0;

		Palette<16> 	mPalette;

		// VRAM
		std::array<uint8_t, kVRAMSizeBytes> mVRAM; // 16 Mb VRAM

		// Higher level state and VDP temporary data
		std::array<uint16_t, kMaximumSpritesCount> mVisibleSpriteIndices;
    	size_t mVisibleSpritesCount = 0;

		std::mutex      mFrameMutex;
		std::mutex      mScanlineMutex;

		// Debug info
		size_t 			mFrameNumber = 0;
};

template <FramebufferDims FBDIMS>
class MsxPPU final: public MsxPPU_BASE {
	public:
		static constexpr uint16_t getPatternsCountPerScreen() {
			return (FBDIMS.width / 8) * (FBDIMS.height / 8);
		}

		MsxPPU(): MsxPPU_BASE() {
			static_assert(divideExact<FBDIMS.width, 16>());
			static_assert(divideExact<FBDIMS.height, 16>());

			setScreenMode(ScreenMode::VSCREEN_5);
		}

		~MsxPPU() = default;

	public:
		bool init();
		bool deinit();
		bool render(uint8_t* pFrameData, uint32_t stride_bytes);
		void renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) override;

		static constexpr uint16_t getScreenWidth() { return FBDIMS.width; }
		static constexpr uint16_t getScreenHeight() { return FBDIMS.height; }

	public:
		// Actual Virtual(V)(TMS)99x8 VDP API

		[[nodiscard]] static constexpr uint32_t getVramPageSize() {
			return FBDIMS.width * FBDIMS.height;
		}

		[[nodiscard]] static constexpr uint32_t getVramPagesCount() {
			return kVRAMSizeBytes / getVramPageSize();
		}

		void setCurrentVramPageIndex(uint8_t index) {
			static_assert(getVramPagesCount() * FBDIMS.width * FBDIMS.height <= kVRAMSizeBytes );
			mCurrentPageIndex = index % getVramPagesCount();
		}

		[[nodiscard]] inline uint8_t getCurrentVramPageIndex() const {
			return mCurrentPageIndex;
		}
		
		// We are doing smooth V9958 style scrolling all the time
		// Horizontal scroll offset (0 to FBDIMS.width pixels)
		void setScrollX(uint16_t absolute_x /* R26 + R27 equivalent */) {
			static constexpr auto sMaxScrollX = FBDIMS.width * 2;
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mScrollX = absolute_x % sMaxScrollX;
		}

		[[nodiscard]] inline uint16_t getScrollX() const { return mScrollX; }

		// We are doing smooth V9958 style scrolling all the time
		// Vertical scroll offset (0 to FBDIMS.height pixels)
		void setScrollY(uint16_t absolute_y /* R22 + R23 equivalent */) {
			static constexpr auto sMaxScrollY = FBDIMS.height * 2;
			const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
			mScrollY = absolute_y % sMaxScrollY;
		}

		[[nodiscard]] inline uint16_t getScrollY() const { return mScrollY; }

		constexpr uint32_t getVramPageAddress(uint16_t page_index) const {
			return page_index * FBDIMS.width * FBDIMS.height; // we just use one byte per pixel no matter what SCREEN is set.
		}

		constexpr uint8_t* getVramPagePtr(uint16_t page_index) {
			return &mVRAM[getVramPageAddress(page_index)]; // we just use one byte per pixel no matter what SCREEN is set.
		}
	
		// "Blitter" functions

		void cmdHMMM(uint16_t source_x, uint16_t source_y, uint16_t dest_x, uint16_t dest_y, uint16_t block_width, uint16_t block_height, bool TMDbit = false /* index 0 color key */);

		// VRAM access

		uint32_t getNameTableSize() const override {
			switch(mScreenMode) {
				case MsxPPU_BASE::ScreenMode::VSCREEN_0:
				case MsxPPU_BASE::ScreenMode::VSCREEN_1:
				case MsxPPU_BASE::ScreenMode::VSCREEN_2:
				case MsxPPU_BASE::ScreenMode::VSCREEN_3:
				case MsxPPU_BASE::ScreenMode::VSCREEN_4:
					return getPatternsCountPerScreen() * 2; // 16 bit indices
				default:
					return 0;
			}
		}

	// Higher level utility functions	
	public:
		bool loadIndexedImagePNG(const std::string& filename, uint32_t vram_address, uint16_t img_width, uint16_t img_height, Palette<16>* pPalette = nullptr);

		void createDefaultMemoryLayout() {
			// reserve memory for 4 screens
			static constexpr uint32_t off_screen_pages_offset = getVramPageSize() * 4;

			uint32_t current_mem_offset = off_screen_pages_offset;
			std::cout << "PatternTable address " << current_mem_offset << std::endl;
			setPatternTableAddress(current_mem_offset);

			current_mem_offset += getPatternsTableSize();
			std::cout << "NameTable address " << current_mem_offset << std::endl;
			setNameTableAddress(current_mem_offset);

			current_mem_offset += getNameTableSize();
			std::cout << "ColorTable address " << current_mem_offset << std::endl;
			setColorTableAddress(current_mem_offset);

			current_mem_offset += geColorTableSize();
			std::cout << "SpritePatternTable address " << current_mem_offset << std::endl;
			setSpritePatternTableAddress(current_mem_offset);

			current_mem_offset += getSpritePatternsTableSize();
			std::cout << "SpriteAttributeTable address " << current_mem_offset << std::endl;
			setSpriteAttributeTableAddress(current_mem_offset);

			current_mem_offset += getSpriteAttributeTableSize();
			std::cout << "SpriteColorTable address " << current_mem_offset << std::endl;
			setSpriteColorTableAddress(current_mem_offset);
		}

		void clearNameTable() {
			const auto name_table_size = getNameTableSize();
			if(name_table_size == 0) return;
			std::memset(&mVRAM[getNameTableAddress()], 0, name_table_size);
		}

		/**
 		* Pushes a complete 4-byte Attribute structure for a single sprite ID into VRAM.
 		*/
		inline Sprite& getSpriteAttribute(uint16_t sprite_id) {
			return *reinterpret_cast<Sprite*>(&mVRAM[getSpriteAttributeTableAddress() + sprite_id * sizeof(Sprite)]);
		}

		inline const uint8_t* getSpritePatternAddress(uint16_t sprite_id) {
			return &mVRAM[getSpritePatternTableAddress() + getSpriteAttribute(sprite_id).index * 8];
		}

		/**
		* Sets all sprite attributes Y to kVerticalTerminatorCode. This tells VPD to stop sprites processing. 
		*/
		inline void clearAllSpriteAttributes() {
			for(uint16_t sprite_id = 0; sprite_id < kMaximumSpritesCount; ++sprite_id) {
				Sprite& sprite = getSpriteAttribute(sprite_id);
				sprite.y = kVerticalTerminatorCode;
			}
		}

	private:
		void outputBlankScreen(uint8_t*pFrameData, uint32_t stride_bytes);

		bool render_SCREEN_2(uint8_t* pFrameData, uint32_t stride_bytes);
		bool render_SCREEN_5(uint8_t* pFrameData, uint32_t stride_bytes);

		FORCE_INLINE void sortSprites() {
			mVisibleSpritesCount = 0;

			const uint8_t sprite_extent = getCurrentSpritesExtent();
			const Sprite* pSprites = reinterpret_cast<const Sprite*>(&mVRAM[getSpriteAttributeTableAddress()]);

			UNROLL_64
			for(uint16_t sprite_id = 0; sprite_id < kMaximumSpritesCount; ++sprite_id) {
				const Sprite& sprite = getSpriteAttribute(sprite_id); //pSprites[sprite_id];
				if(sprite.y == kVerticalTerminatorCode) return;

				if( (sprite.x >= FBDIMS.width) || ((sprite.x + sprite_extent) < 0)) continue;

				mVisibleSpriteIndices[mVisibleSpritesCount] = sprite_id;
				mVisibleSpritesCount++;
			}

			if(mVisibleSpritesCount > 0 && mVisibleSpritesCount <= mVisibleSpriteIndices.size()) {
        		std::reverse(mVisibleSpriteIndices.begin(), mVisibleSpriteIndices.begin() + mVisibleSpritesCount);
    		}
		}

		template<ScreenMode SCREEN_MODE>
		inline void render_SPRITES_LINE(uint16_t line, uint8_t* pFrameData, uint32_t stride_bytes) {
			if(mSpritesDisableFlag || mVisibleSpritesCount == 0) return;
			
			// Render sprites
			//std::cout << "Visible sprites count " << mVisibleSpritesCount << std::endl;

			uint32_t* pDstLine = reinterpret_cast<uint32_t*>(pFrameData + line * stride_bytes);
			const int16_t sprite_extent = static_cast<int16_t>(getCurrentSpritesExtent());

			for(uint16_t i = 0; i < mVisibleSpritesCount; ++i) {
				const Sprite& sprite = getSpriteAttribute(mVisibleSpriteIndices[i]);
				if((int)line < sprite.y || ((int)line >= sprite.y + sprite_extent)) continue;

				uint16_t pattern_line = line - sprite.y;
				const uint8_t* pPatternData = getSpritePatternAddress(mVisibleSpriteIndices[i]) + pattern_line;

				uint8_t color_index = sprite.attribs.color;

				if constexpr (SCREEN_MODE != ScreenMode::VSCREEN_1 && SCREEN_MODE != ScreenMode::VSCREEN_2 && SCREEN_MODE != ScreenMode::VSCREEN_3) {
					assert(false && "Fetch color from table");
				} 

				uint16_t pattern_word = *pPatternData << 8;
				if(mSpriteSize == SpriteSize::SPRITE_16) {		
					pattern_word |= *(pPatternData + 16);
				}

				uint16_t screen_start_x = std::min(sprite.x < 0 ? 0 : sprite.x, FBDIMS.width - 1);
				uint16_t sprite_start_x = sprite.x < 0 ? (-sprite.x) : 0;
				uint16_t sprite_visible_pixels_count = (((sprite.x + sprite_extent) >= FBDIMS.width) ? (FBDIMS.width - sprite.x) : sprite_extent) - sprite_start_x;

				for(uint16_t x = screen_start_x; x < (screen_start_x + sprite_visible_pixels_count); ++x) {
					if(pattern_word >> (15 - ((sprite_start_x++))) & 1) {
						if(color_index != 0) *(pDstLine + x) = mPalette[color_index] >> 8;
					}
				}
			}
		}
};

}  // namespace PPU

}  // namespace RetroCore

inline std::string to_string(const RetroCore::PPU::MsxPPU_BASE::ScreenMode& mode) {
	using ScreenMode = RetroCore::PPU::MsxPPU_BASE::ScreenMode;

	switch(mode) {
		case ScreenMode::VSCREEN_1:
			return "VSCREEN_1";
		case ScreenMode::VSCREEN_2:
			return "VSCREEN_2";
		case ScreenMode::VSCREEN_3:
			return "VSCREEN_3";
		case ScreenMode::VSCREEN_4:
			return "VSCREEN_4";
		case ScreenMode::VSCREEN_5:
			return "VSCREEN_5";
		case ScreenMode::VSCREEN_6:
			return "VSCREEN_6";
		case ScreenMode::VSCREEN_7:
			return "VSCREEN_7";
		case ScreenMode::VSCREEN_8:
			return "VSCREEN_8";
		case ScreenMode::VSCREEN_10:
			return "VSCREEN_10";
		case ScreenMode::VSCREEN_11:
			return "VSCREEN_11";
		case ScreenMode::VSCREEN_12:
			return "VSCREEN_12";
		default:
			assert(false && "Should not be here!");
			return "Unknown screen";
	}
}

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_H