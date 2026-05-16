#include "framework/palette.h"

#include <cstring>
#include <limits>
#include <thread>


namespace RetroCore {

template<uint32_t CNT>
constexpr Palette<64> createNESPalette(const std::array<uint8_t, 192>& a) {
	static_assert(CNT == 64);
	Palette<64> palette;

    for(size_t i = 0; i < 64; ++i) {
    	size_t ii = i * 3;
    	palette.mColors[i] = {a[ii++], a[ii++], a[ii], 255};
    }

    return std::move(palette);
}

bool loadNESPalette(const std::string& filename, Palette<64>& palette) {
	std::ifstream file(filename, std::ios::binary);
    
    if (!file) {
        std::cerr << "Could not open palette file: " << filename << std::endl;
        return false;
    }

    // Read 192 bytes (64 colors * 3 bytes each) directly into the array
    std::array<uint8_t, 192> nes_values;

    file.read(reinterpret_cast<char*>(nes_values.data()), 192);

    if (file.gcount() != 192) {
        std::cerr << "Invalid NES palette file size (expected 192 bytes)." << std::endl;
        return false;
    }

    palette = createNESPalette(nes_values);

    return true;
}


}  // namespace RetroCore