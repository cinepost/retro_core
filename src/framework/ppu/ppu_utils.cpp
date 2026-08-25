#include "framework/ppu/ppu_utils.h"

#include "lodepng/lodepng.h"

#include <iostream>
#include <vector>
#include <array>
#include <string>


namespace RetroCore {
namespace PPU {
namespace Utils {

template <size_t COLOR_COUNT>
[[nodiscard]] bool loadIndexedPng(const std::string& filename, uint16_t img_width, uint16_t img_height, std::vector<uint8_t>& img_out_data, Palette<COLOR_COUNT>* pOutPalette) {
    lodepng::State state;

    // CRITICAL: Prevent LodePNG from auto-converting the output to RGBA.
    // Forcing PALETTE color type with 8-bit depth ensures 1 byte per pixel output.
    
    state.info_raw.bitdepth = 8;
    state.decoder.color_convert = 0; // Do not convert indexed color to rgba

    std::vector<unsigned char> imageFileBytes;
    unsigned int width = 0;
    unsigned int height = 0;

    // Load the PNG file from disk into memory
    if (lodepng::load_file(imageFileBytes, filename) != 0) {
        std::cerr << "Error loadIndexedPng(): Failed to open file " << filename << "\n";
        return false;
    }

    // Decode the file bytes into a temporary raw vector conforming to state.info_raw
    std::vector<unsigned char> decodedPixels;
    unsigned int error = lodepng::decode(decodedPixels, width, height, state, imageFileBytes);
    
    if (error) {
        std::cerr << "Error loadIndexedPng(): LodePNG decoder error " << error << ": " << lodepng_error_text(error) << "\n";
        return false;
    }

    // Size validation check against known fixed bounds
    if (width != img_width || height != img_height || decodedPixels.size() != (img_width * img_height)) {
        std::cerr << "Error loadIndexedPng(): Actual image dimensions (" << width << "x" << height << ") do not match the expected image size (" << img_width << "x" << img_height << ").\n";
        return false;
    }

    // Safely copy the pixel index bytes into the std::array
    img_out_data.resize(decodedPixels.size());
    std::copy(decodedPixels.begin(), decodedPixels.end(), img_out_data.data());

    if(pOutPalette) {
        // Extract palette values from the source PNG info block
        // Accessing state.info_png (the metadata inside the file) rather than info_raw
        size_t paletteSize = state.info_png.color.palettesize;

        for (size_t i = 0; i < paletteSize && i < pOutPalette->size(); ++i) {
            // quantize to 333 and back
            const RGBA8888 color(state.info_png.color.palette[i * 4], state.info_png.color.palette[i * 4 + 1], state.info_png.color.palette[i * 4 + 2], state.info_png.color.palette[i * 4 + 3]);
            pOutPalette->setColor(i, color);
        }
    }

    return true;
}

template [[nodiscard]] bool loadIndexedPng<16>(const std::string& filename, uint16_t img_width, uint16_t img_height, std::vector<uint8_t>& img_out_data, Palette<16>* pOutPalette);

}  // namespace Utils
}  // namespace PPU
}  // namespace RetroCore