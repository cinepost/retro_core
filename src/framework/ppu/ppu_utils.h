#ifndef __RETRO_CORE_FRAMEWORK_PPU_PPU_UTILS_H
#define __RETRO_CORE_FRAMEWORK_PPU_PPU_UTILS_H

#include "framework/palette.h"
#include "framework/ppu/ppu_msx.h"


namespace RetroCore {
namespace PPU {
namespace Utils {

template <size_t COLOR_COUNT>
[[nodiscard]] bool loadIndexedPng(const std::string& filename, uint16_t img_width, uint16_t img_height, uint8_t* pDst, Palette<COLOR_COUNT>& outPalette);

}  // namespace Utils
}  // namespace PPU
}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_UTILS_H