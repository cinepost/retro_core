#include "framework/ppu/ppu_nes.h"

namespace RetroCore {

namespace PPU {

static_assert(sizeof(NesPPU_BASE::CHRTile) == 16);
static_assert(sizeof(NesPPU_BASE::PatternTable) == 65536 * 16); // 1mb of CHR tiles data

template <FramebufferDims FBDIMS>
bool NesPPU<FBDIMS>::init() {
	return true;
}

template <FramebufferDims FBDIMS>
bool NesPPU<FBDIMS>::deinit() {
	return true;
}

template <FramebufferDims FBDIMS>
bool NesPPU<FBDIMS>::render(uint8_t* pFrameData, uint32_t stride_bytes) {
	static_assert(FBDIMS.height > 0);
	for (uint16_t scanline = 0; scanline < FBDIMS.height; ++scanline) {

		uint16_t scroll_y = mScrollY + scanline;
		uint16_t scroll_x = mScrollX;
		        
        // Per-scanline callback if registered one
        if (mScanlineCallback != nullptr) {
            mScanlineCallback(scanline, scroll_x, scroll_y);
        }

        static constexpr uint16_t s_x_scroll_mask = getIndexMask<uint16_t, uint16_t>(FBDIMS.width);
        static constexpr uint16_t s_y_scroll_mask = getIndexMask<uint16_t, uint16_t>(FBDIMS.height);

        scroll_x &= s_x_scroll_mask;
        scroll_y &= s_y_scroll_mask;

        {
        	const std::lock_guard<std::mutex> lock_scanline(mScanlineMutex);
        	const std::lock_guard<std::mutex> lock_cram(mCRAMMutex);
			const std::lock_guard<std::mutex> lock_nametables(mNametablesMutex);

        // Render the line using the potentially modified 'scroll_y' register
    	//render_scanline(framebuffer, scanline);
        	uint8_t* pLineData = pFrameData + scanline * stride_bytes;
        	pLineData[100] = 255;
        	pLineData[101] = 255;
        	pLineData[102] = 255;
        	//pLineData[403] = 255;
        }
    }
	
	return true;
}

template class CRAM<NesPPU_BASE::CRAMEntryType, 4, 2>;

template class Abstract_PPU<Platform::NES>;
template class NesPPU<{512, 288}>;

}  // namespace PPU

}  // namespace RetroCore