#include "framework/ppu/ppu_msx_utils.h"

#include "lodepng/lodepng.h"

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <set>
#include <type_traits>

namespace RetroCore {
namespace PPU {
namespace Utils {
namespace MSX {

template<typename T>
[[nodiscard]] std::vector<T> loadTilesFromIndexedPNG(const uint8_t* pData, size_t data_size, const Palette<16>* pRefPalette, bool skip_empty_tiles) { 
    using PATTERN_8D = PPU::MsxPPU_BASE::PATTERN_8D;
    using PATTERN_32D = PPU::MsxPPU_BASE::PATTERN_32D;
    using PATTERN_8D_8C = PPU::MsxPPU_BASE::PATTERN_8D_8C;

    static_assert(std::is_same_v<T, PATTERN_8D_8C> || std::is_same_v<T, PATTERN_8D> || std::is_same_v<T, PATTERN_32D>);

    // Configure State structures to preserve raw 8-bit index properties
    lodepng::State state;
    state.info_raw.colortype = LCT_PALETTE;
    state.info_raw.bitdepth = 8;

    unsigned int imgWidth = 0;
    unsigned int imgHeight = 0;

    std::vector<unsigned char> rawPixels; // Expect 8bpp indexed image
    unsigned error = lodepng::decode(rawPixels, imgWidth, imgHeight, state, reinterpret_cast<const unsigned char*>(pData), data_size);

    if (error) {
        std::cerr << "LodePNG error " << error << ": " << lodepng_error_text(error) << std::endl;
        return {};
    }

    const uint32_t tile_width = std::is_same_v<T, PATTERN_32D> ? 16 : 8;
    const uint32_t tile_height = std::is_same_v<T, PATTERN_32D> ? 16 : 8;

    if(rawPixels.empty() || imgWidth == 0 || imgHeight == 0) {
        std::cerr << "Error: Image has no data ! " << std::endl;
        return {};
    }

    // Verify dimension properties perfectly align with uniform 8x8 squares
    if (imgWidth % tile_width != 0 || imgHeight % tile_height != 0) {
        std::cerr << "Error: Image dimensions are not multiples of " << tile_width << "x" << tile_height<< " pixels." << std::endl;
        return {};
    }

    if(rawPixels.size() != (imgWidth * imgHeight)) {
        std::cerr << "Error: Not expected image raw pixels count ! " << rawPixels.size() << std::endl;
        return {};
    }

    // find closest reference palette colors. check image is 1bpp if no palleter is provided
    std::array<uint32_t, 256> image_colors_to_palette;
    if(pRefPalette) {
        for (size_t i = 0; i < state.info_png.color.palettesize; ++i) {
            // quantize to 333 and back
            RGBA8888 color(state.info_png.color.palette[i * 4], state.info_png.color.palette[i * 4 + 1], state.info_png.color.palette[i * 4 + 2], 255);
            color = v9938_to_rgb888(rgb888_to_v9938(color));
            assert(i < image_colors_to_palette.size());
            image_colors_to_palette[i] = pRefPalette->findClosestColorIndex(color, false /* do not include alpha */);
        }
    } else {
        if(state.info_png.color.palettesize != 2) {
            std::cerr << "Error: Only 1BPP images are supported if no reference palette is provided!" << std::endl;
            return {};
        }
    }

    std::vector<T> outputTiles;
    const size_t tilesX = imgWidth / tile_width;
    const size_t tilesY = imgHeight / tile_height;

    assert(tile_width == tile_height); // only square tiles 
    const uint32_t subtiles_count = tile_width >> 3;

    for (size_t ty = 0; ty < tilesY; ++ty) {
        for (size_t tx = 0; tx < tilesX; ++tx) {

            T currentTile;
            currentTile.tile.fill(85);

            bool output_tile_is_empty = false;

            // fetch one 8x8 local block for 8x8 sprites and tiles and four for 16x16 sprites
            for(uint32_t sy = 0; sy < subtiles_count; ++sy) {
                for(uint32_t sx = 0; sx < subtiles_count; ++sx) {

                    // Phase 1: Localize the 8x8 palette index data block
                    std::array<std::array<uint8_t, 8>, 8> localBlock{};
                    uint8_t globalFirstPixel = rawPixels[((ty * tile_height + sy * 8) * imgWidth) + (tx * tile_width + sx * 8)];
                    bool isUniformColor = true;

                    for (size_t y = 0; y < 8; ++y) {
                        for (size_t x = 0; x < 8; ++x) {
                            std::set<uint8_t> unique_colors;

                            size_t pixelIndex = ((ty * tile_height + y + sy * 8) * imgWidth) + (tx * tile_width + x + sx * 8);
                            uint8_t colorIndex = rawPixels[pixelIndex];
                            localBlock[y][x] = colorIndex;
                            unique_colors.insert(colorIndex);

                            if (unique_colors.size() > 2) {
                                std::cerr << "Error: tile[" << tx << "][" << ty << "] has more than 2 colors in row " << y << " !" << std::endl;
                                return {};
                            }

                            if (colorIndex != globalFirstPixel) {
                                isUniformColor = false;
                            }
                        }
                    }

                    // Skip empty tiles comprised purely of one static baseline color index
                    if (isUniformColor && skip_empty_tiles) {
                        output_tile_is_empty = true;
                        continue; 
                    }

                    bool totalTileIsZeroes = true;
                    bool totalTileIsOnes = true;

                    // Transpile the row properties into bits and 4-bit colors
                    for (size_t y = 0; y < 8; ++y) {
                        uint8_t firstColorInRow = localBlock[y][0];
                        uint8_t secondaryColorInRow = firstColorInRow; 
                        
                        // Track colors inside the line to fill high/low color nibbles
                        for (size_t x = 0; x < 8; ++x) {
                            if (localBlock[y][x] != firstColorInRow) {
                                secondaryColorInRow = localBlock[y][x];
                                break;
                            }
                        }

                        bool color_swap = false;
                        if(!pRefPalette && firstColorInRow > secondaryColorInRow) {
                            auto tmp = secondaryColorInRow;
                            secondaryColorInRow = firstColorInRow;
                            firstColorInRow = tmp;
                            color_swap = true;
                        }


                        uint8_t tileRowByte = 0;
                        for (size_t x = 0; x < 8; ++x) {
                            uint8_t currentPixel = localBlock[y][x];
                            
                            if(!pRefPalette){
                                if(currentPixel != 0x00) tileRowByte |= (1 << (7 - x)); // MSB layout mapping
                            } else {
                                assert(false && "not implemented");
                            }
                        }

                        // Check uniform content parameters
                        if (tileRowByte != 0x00) totalTileIsZeroes = false;
                        if (tileRowByte != 0xFF) totalTileIsOnes = false;

                        currentTile.tile[y + sy*8 + sx*16] = tileRowByte;
                        
                        // Store low nibble (Color 0) and high nibble (Color 1) in 4-bit layouts
                        if constexpr(std::is_same_v<T, PPU::MsxPPU_BASE::PATTERN_8D_8C>) {
                            if(pRefPalette) {
                                currentTile.color[y] = (firstColorInRow & 0x0F) | ((secondaryColorInRow & 0x0F) << 4);
                            } else {
                                currentTile.color[y] = 0xF0;
                            }
                        }
                    }

                    // Final validation filter checking computed bit patterns
                    if (totalTileIsZeroes || totalTileIsOnes) {
                        output_tile_is_empty = true; 
                    }
                }
            }

            if(output_tile_is_empty && skip_empty_tiles) continue;

            outputTiles.push_back(currentTile);
        }
    }

    return outputTiles;
}

/**
 * @brief Parses an indexed PNG file into PPU::MsxPPU_BASE::PATTERN_8D_8C or PPU::MsxPPU_BASE::PATTERN_8D structures.
 * @param filename PNG image file path.
 * @param pRefPalette Optional reference palette to ind exact or closest color in.
 * @param skip_empty_tiles Skip empty or totally filled tiles.
 * @return Vector filled with non-empty parsed elements.
 */

template<typename T>
[[nodiscard]] std::vector<T> loadTilesFromIndexedPNG(const std::string& filename, const Palette<16>* pRefPalette, bool skip_empty_tiles) { 
    std::vector<unsigned char> imageFileBytes;
    unsigned int width = 0;
    unsigned int height = 0;

    // 1. Load the PNG file from disk into memory
    if (lodepng::load_file(imageFileBytes, filename) != 0) {
        std::cerr << "Error loadTilesFromIndexedPNG(): Failed to open file " << filename << "\n";
        return {};
    }

    return loadTilesFromIndexedPNG<T>(imageFileBytes.data(), imageFileBytes.size(), pRefPalette, skip_empty_tiles);
}

}  // namespace MSX
}  // namespace Utils
}  // namespace PPU
}  // namespace RetroCore

template std::vector<RetroCore::PPU::MsxPPU_BASE::PATTERN_8D> RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG(const std::string&, const RetroCore::Palette<16>*, bool);
template std::vector<RetroCore::PPU::MsxPPU_BASE::PATTERN_32D> RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG(const std::string&, const RetroCore::Palette<16>*, bool);
template std::vector<RetroCore::PPU::MsxPPU_BASE::PATTERN_8D_8C> RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG(const std::string&, const RetroCore::Palette<16>*, bool);

template std::vector<RetroCore::PPU::MsxPPU_BASE::PATTERN_8D> RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG(const uint8_t*, size_t, const RetroCore::Palette<16>*, bool);
template std::vector<RetroCore::PPU::MsxPPU_BASE::PATTERN_32D> RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG(const uint8_t*, size_t, const RetroCore::Palette<16>*, bool);
template std::vector<RetroCore::PPU::MsxPPU_BASE::PATTERN_8D_8C> RetroCore::PPU::Utils::MSX::loadTilesFromIndexedPNG(const uint8_t*, size_t, const RetroCore::Palette<16>*, bool);