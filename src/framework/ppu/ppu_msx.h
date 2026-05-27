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

		enum class Mode {
			TMS9918A = 0,
			V9938,
			V9958
		};

		MsxPPU_BASE(): Abstract_PPU<Platform::MSX>() {

		}

	public:


	protected:

};

template <FramebufferDims FBDIMS, MsxPPU_BASE::Mode MODE>
class MsxPPU: public MsxPPU_BASE {
	public:
		static constexpr Mode mMode = MODE;

	public:
		bool init();
		bool deinit();
		bool render(uint8_t* pFrameData, uint32_t stride_bytes);
		void renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) override;
};

}  // namespace PPU

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_H