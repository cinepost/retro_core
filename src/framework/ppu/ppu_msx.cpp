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
	static_assert(FBDIMS.height > 0);
	const std::lock_guard<std::mutex> lock_frame(mFrameMutex);

	mCollisionDetected = false;


	mFrameNumber++;
	return true;
}

template <FramebufferDims FBDIMS, MsxPPU_BASE::Mode MODE>
void MsxPPU<FBDIMS, MODE>::renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) {

}

template class Abstract_PPU<Platform::MSX>;

template class MsxPPU<{512, 288}, MsxPPU_BASE::Mode::V_MODE1>;
template class MsxPPU<{512, 288}, MsxPPU_BASE::Mode::V_MODE2>;

}  // namespace PPU

}  // namespace RetroCore