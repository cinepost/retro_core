#include "framework/ppu/ppu_msx.h"


namespace RetroCore {

namespace PPU {


template <FramebufferDims FBDIMS, MsxPPU_BASE::Mode MODE>
bool MsxPPU<FBDIMS, MODE>::init() {
	return true;
}

template <FramebufferDims FBDIMS, MsxPPU_BASE::Mode MODE>
bool MsxPPU<FBDIMS, MODE>::deinit() {
	return true;
}

template <FramebufferDims FBDIMS, MsxPPU_BASE::Mode MODE>
bool MsxPPU<FBDIMS, MODE>::render(uint8_t* pFrameData, uint32_t stride_bytes) {

	return true;
}

template <FramebufferDims FBDIMS, MsxPPU_BASE::Mode MODE>
void MsxPPU<FBDIMS, MODE>::renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) {

}

template class Abstract_PPU<Platform::MSX>;

template class MsxPPU<{512, 288}, MsxPPU_BASE::Mode::TMS9918A>;
template class MsxPPU<{512, 288}, MsxPPU_BASE::Mode::V9938>;
template class MsxPPU<{512, 288}, MsxPPU_BASE::Mode::V9958>;

}  // namespace PPU

}  // namespace RetroCore