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
[[nodiscard]] bool loadIndexedPng(const std::string& filename, uint32_t img_width, uint32_t img_height, std::vector<uint8_t>& img_out_data, Palette<COLOR_COUNT>* pOutPalette) {
    // CRITICAL: Prevent LodePNG from auto-converting the output to RGBA.
    // Forcing PALETTE color type with 8-bit depth ensures 1 byte per pixel output.

    img_out_data.resize(img_width * img_height);
    
    std::vector<unsigned char> imageFileBytes;
    unsigned int width = 0;
    unsigned int height = 0;

    // Load the PNG file from disk into memory
    if (lodepng::load_file(imageFileBytes, filename) != 0) {
        std::cerr << "Error loadIndexedPng(): Failed to open file " << filename << "\n";
        return false;
    }

    // Inspect the header without decompressing pixel data
    lodepng::State state;
    unsigned error = lodepng_inspect(&width, &height, &state, imageFileBytes.data(), imageFileBytes.size());
    
    if (error) {
        std::cerr << "Error loadIndexedPng(): " << filename << " inspection error " << error << ": " << lodepng_error_text(error) << std::endl;
        return false;
    }

    if(width != img_width || height != img_height) {
        std::cerr << "Error loadIndexedPng(): Unexpected image " << filename << " size requested " << img_width << "x" << img_height << ". Actual image size is (" << width << "x" << height << ")" << std::endl;
        return false;
    }

    return loadIndexedPng<COLOR_COUNT>(static_cast<const uint8_t*>(imageFileBytes.data()), imageFileBytes.size(), img_width, img_height, img_out_data, pOutPalette); 
}

template <size_t COLOR_COUNT>
[[nodiscard]] bool loadIndexedPng(const uint8_t* pData, size_t data_size, uint32_t img_width, uint32_t img_height, std::vector<uint8_t>& img_out_data, Palette<COLOR_COUNT>* pOutPalette) {
    if(pData == nullptr || data_size == 0) return false;
    lodepng::State state;

    // CRITICAL: Prevent LodePNG from auto-converting the output to RGBA.
    // Forcing PALETTE color type with 8-bit depth ensures 1 byte per pixel output.
    
    state.info_raw.bitdepth = 8;
    state.decoder.color_convert = 0; // Do not convert indexed color to rgba

    unsigned int width = 0;
    unsigned int height = 0;

    // Decode the file bytes into a temporary raw vector conforming to state.info_raw
    std::vector<unsigned char> decodedPixels;
    unsigned error = lodepng::decode(decodedPixels, width, height, state, reinterpret_cast<const unsigned char*>(pData), data_size);
    
    if (error) {
        std::cerr << "Error loadIndexedPng(): LodePNG decoder error " << error << ": " << lodepng_error_text(error) << "\n";
        return false;
    }

    // Size validation check against known fixed bounds
    if(width != img_width || height != img_height) {
        std::cerr << "Error loadIndexedPng(): Unexpected png image size requested " << img_width << "x" << img_height << ". Actual image size is (" << width << "x" << height << ")" << std::endl;
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

template [[nodiscard]] bool loadIndexedPng<16>(const std::string& filename, uint32_t img_width, uint32_t img_height, std::vector<uint8_t>& img_out_data, Palette<16>* pOutPalette);
template [[nodiscard]] bool loadIndexedPng<16>(const uint8_t* pData, size_t data_size, uint32_t img_width, uint32_t img_height, std::vector<uint8_t>& img_out_data, Palette<16>* pOutPalette);

}  // namespace Utils
}  // namespace PPU
}  // namespace RetroCore