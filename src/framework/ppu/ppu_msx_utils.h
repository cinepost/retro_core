#ifndef __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_UTILS_H
#define __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_UTILS_H

#include "framework/palette.h"
#include "framework/ppu/ppu_msx.h"


namespace RetroCore {
namespace PPU {
namespace Utils {
namespace MSX {

[[nodiscard]] std::vector<PPU::MsxPPU_BASE::PATTERN_8D_8C> loadTilesFromIndexedPNG(const std::string& filename, const Palette<16>* pRefPalette = nullptr, bool skip_empty_tiles = true);

}  // namespace MSX
}  // namespace Utils
}  // namespace PPU
}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_UTILS_H