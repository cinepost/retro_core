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


namespace RetroCore {

namespace StaticData { 
namespace MSX {

// Some static MSX PPU related data here..

}  // namespace MSX
}  // namespace StaticData

namespace PPU {

class MsxPPU_BASE: public Abstract_PPU<Platform::MSX> {
	public:
		static const uint16_t kMaximumSpriteIndex = 1023;
		
		using CollisionDetectionCallback = std::function<void(uint16_t&, uint16_t&)>;

		enum class Mode {
			V_MODE1 = 0, // Virtual Legacy Mode (TMS9918A Compatibility)
			V_MODE2,     // Virtual Advanced Mode (The Native V9938/V9958 Standard)
			UNKNOWN
		};

		struct alignas(8) Sprite {
			static constexpr uint16_t 	kInvalidTileIndex = std::numeric_limits<uint16_t>::max();
			static constexpr int16_t 	kOffScreenPos = std::numeric_limits<int16_t>::min();
			int16_t x;
			int16_t y;
			uint16_t tile_index;
			uint8_t attibs;
			uint8_t _pad;
		};

		using SAT = std::array<Sprite, kMaximumSpriteIndex + 1>;

	public:
		// Set per-scanline callback
    	void setCollisionDetectionCallback(CollisionDetectionCallback cb) {
    		const std::lock_guard<std::mutex> lock_frame(mFrameMutex);
        	mCollisionDetectionCallback = cb;
    	}

		MsxPPU_BASE(): Abstract_PPU<Platform::MSX>() {

		}

	protected:
		CollisionDetectionCallback mCollisionDetectionCallback = nullptr;


		// Internal PPU state
		bool            mSpritesSizeFlag	= false; 	// Equivalent to Bit 1 of Control Register 1 (R1). False - 8x8, True - 16x16.
		bool            mSpritesMag 		= false;	// Equivalent to Bit 0 of Control Register 1 (R1). False - 1x scale, True - 2x scale.
		bool            mSpritesDisableFlag	= false;	// Equivalent to Bit 6 of Register 8 (R8). True - entire sprite rendering pipeline is bypassed.

		bool            mCollisionDetected 	= false;

		//
		size_t 			mFrameNumber = 0;

		std::mutex      mFrameMutex;
		std::mutex      mScanlineMutex;
};

template <FramebufferDims FBDIMS, MsxPPU_BASE::Mode MODE>
class MsxPPU: public MsxPPU_BASE {
	public:
		static constexpr Mode mMode = MODE;
		static constexpr uint16_t kVerticalTerminatorCode = FBDIMS.height; // In V_MODE1 or V_MODE2, if sprite y is equal to kVerticalTerminatorCode must instantly stop processing any subsequent sprites in the table.

		MsxPPU(): MsxPPU_BASE() {
			static_assert(MODE != MsxPPU_BASE::Mode::UNKNOWN);
		}

	public:
		bool init();
		bool deinit();
		bool render(uint8_t* pFrameData, uint32_t stride_bytes);
		void renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) override;
};

}  // namespace PPU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_H