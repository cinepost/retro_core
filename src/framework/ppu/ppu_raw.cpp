#include "framework/ppu/ppu_raw.h"
#include "framework/ppu/ppu_utils.h"

namespace RetroCore {

namespace PPU {


template<typename T>
void RawPPU::vramBlockSet(uint32_t vram_address, const T& value, uint16_t count) {
	for(uint16_t i = 0; i < count; ++i) {
		std::memcpy(&mVRAM[vram_address + sizeof(T) * i], reinterpret_cast<const uint8_t*>(&value), sizeof(T));
	}
}

bool RawPPU::init() {
	std::memset(mVRAM.data(), 0, mVRAM.size() * sizeof(uint8_t));
	return true;
}

bool RawPPU::deinit() {
	return true;
}

void RawPPU::reset() {
	clearVRAM();
}

bool RawPPU::render(uint8_t* pFrameData, uint32_t stride_bytes) {

	return true;
}

void RawPPU::renderDebugScreen(uint8_t* pFrameData, uint32_t stride_bytes) {

}
/*
// Fast unsafe using static array with memcopy
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
inline void fillRectN(uint8_t* pFrameData, uint32_t stride_bytes, uint16_t x, uint16_t y, uint32_t color) {
    for (uint16_t line = 0; line < HEIGHT; ++line) {
        uint32_t* pRowPixel = reinterpret_cast<uint32_t*>(pFrameData + (line + y) * stride_bytes) + x;
        std::fill_n(pRowPixel, WIDTH, color);
    }
}
*/

// Specialization

template class Abstract_PPU<Platform::RAW>;

template void RawPPU::vramBlockSet<uint8_t>(uint32_t vram_address, const uint8_t& value, uint16_t count);
template void RawPPU::vramBlockSet<uint16_t>(uint32_t vram_address, const uint16_t& value, uint16_t count);
template void RawPPU::vramBlockSet<uint32_t>(uint32_t vram_address, const uint32_t& value, uint16_t count);

}  // namespace PPU

}  // namespace RetroCore