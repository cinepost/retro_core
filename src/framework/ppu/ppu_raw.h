#ifndef __RETRO_CORE_FRAMEWORK_PPU_PPU_RAW_H
#define __RETRO_CORE_FRAMEWORK_PPU_PPU_RAW_H

#include "framework/static_data.h"
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


namespace RetroCore {

namespace PPU {

class RawPPU: public Abstract_PPU<Platform::RAW> {
	public:

		static const uint32_t kVRAMSizeBytes = 1024 * 1024 * 16; // 16 Mb VRAM 

	public:
		RawPPU(): Abstract_PPU<Platform::RAW>() {
			setScreenSize(320, 224);
			init();
		}

		virtual void reset() override final;
		virtual bool init() override final;
		virtual bool deinit() override final;
		virtual bool render(uint8_t* pFrameData, uint32_t stride_bytes) override final;
		virtual void renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) override final;

		void setScreenSize(uint16_t width, uint16_t height) {
			assert(width != 0);
			assert(height != 0);
			if(mScreenWidth == width && mScreenHeight == height) return;
			mVRAM.resize(mScreenWidth * mScreenHeight * bytesPerPixel(mNativeFramebufferPixelFormat));
		}

		uint16_t getScreenWidth() { return mScreenWidth; }
		uint16_t getScreenHeight() { return mScreenHeight; }

	private:
		void outputBlankScreen(uint8_t*pFrameData, uint32_t stride_bytes);

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


	protected:
		uint16_t mScreenWidth;
		uint16_t mScreenHeight;

		std::vector<uint8_t> mVRAM;
		// Debug info
		size_t 			mFrameNumber = 0;
};

}  // namespace PPU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_H