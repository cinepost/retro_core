#ifndef __RETRO_CORE_FRAMEWORK_PPU_PPU_UTILS_H
#define __RETRO_CORE_FRAMEWORK_PPU_PPU_UTILS_H

#include "framework/palette.h"


namespace RetroCore {
namespace PPU {
namespace Utils {

template <size_t COLOR_COUNT>
[[nodiscard]] bool loadIndexedPng(const uint8_t* pData, size_t data_size, uint32_t img_width, uint32_t img_height, std::vector<uint8_t>& img_out_data, Palette<COLOR_COUNT>* pOutPalette = nullptr);

template <size_t COLOR_COUNT>
[[nodiscard]] bool loadIndexedPng(const std::string& filename, uint32_t img_width, uint32_t img_height, std::vector<uint8_t>& img_out_data, Palette<COLOR_COUNT>* pOutPalette = nullptr);

}  // namespace Utils
}  // namespace PPU
}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_UTILS_H