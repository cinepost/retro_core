#ifndef __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_UTILS_H
#define __RETRO_CORE_FRAMEWORK_PPU_PPU_MSX_UTILS_H

#include "framework/palette.h"
#include "framework/ppu/ppu_msx.h"


namespace RetroCore {
namespace PPU {
namespace Utils {
namespace MSX {

#define _HV(x) 0x##x

#define MSX_MAKE_PATTERN_8D_8C(p0, p1, p2, p3, p4, p5, p6, p7, c0, c1, c2, c3, c4, c5, c6, c7) \
    RetroCore::PPU::MsxPPU_BASE::PATTERN_8D_8C( \
        std::array<uint8_t, 8>{_HV(p0), _HV(p1), _HV(p2), _HV(p3), _HV(p4), _HV(p5), _HV(p6), _HV(p7)}, \
        std::array<uint8_t, 8>{_HV(c0), _HV(c1), _HV(c2), _HV(c3), _HV(c4), _HV(c5), _HV(c6), _HV(c7)}  \
    )

template<typename T>
[[nodiscard]] std::vector<T> loadTilesFromIndexedPNG(const std::string& filename, const Palette<16>* pRefPalette = nullptr, bool skip_empty_tiles = false);

template<typename T>
[[nodiscard]] std::vector<T> loadTilesFromIndexedPNG(const uint8_t* pData, size_t data_size, const Palette<16>* pRefPalette = nullptr, bool skip_empty_tiles = false);

}  // namespace MSX
}  // namespace Utils
}  // namespace PPU
}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PPU_PPU_UTILS_H

/*
    16x16 sprite pattern data memory layout

        Left 8 Pixels   Right 8 Pixels
      +---------------+----------------+
Row  0| Block 0       | Block 2        |
  to  | (Top-Left)    | (Top-Right)    |
Row  7| Bytes 0 to 7  | Bytes 16 to 23 |
      +---------------+----------------+
Row  8| Block 1       | Block 3        |
  to  | (Bottom-Left) | (Bottom-Right) |
Row 15| Bytes 8 to 15 | Bytes 24 to 31 |
      +---------------+----------------+

*/