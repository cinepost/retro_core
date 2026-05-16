#ifndef __RETRO_CORE_FRAMEWORK_PALETTE_H
#define __RETRO_CORE_FRAMEWORK_PALETTE_H

#include "framework/formats.h"

#include <cstdint>
#include <vector>
#include <cassert>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>


namespace RetroCore {

struct PaletteBase {
	using Color = std::array<uint8_t, 4>;
};

template<uint32_t CNT>
class Palette: public PaletteBase {
	public:
		Palette(Palette<CNT>&& other) noexcept {
			mColors = std::move(other.mColors);
		}

		void operator=(Palette<CNT>&& other) noexcept {
			if (this != &other) {
				mColors = std::move(other.mColors);
			}
		}

		virtual const std::array<Color, CNT>& getColors() const { return mColors; }

		const Color& getColor(uint16_t index) const {
			static_assert(CNT >= 65536);
			return mColors[index];
		}

		const Color& getNESColor(uint8_t index) const {
			static_assert(CNT == (64), "NES palette should have 64 colors!");
			return mColors[index & 0x3F];
		}

	private:
		Palette() {};

	private:
		std::array<Color, CNT> mColors;
};

Palette<64> createNESPalette(const std::array<uint8_t, 192>& a);

bool loadNESPalette(const std::string& filename, Palette<64>& palette);

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PALETTE_H