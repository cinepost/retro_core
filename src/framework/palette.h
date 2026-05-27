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
#include <bit>

namespace RetroCore {

struct PaletteBase {
	using Color = uint32_t;
};

template<uint32_t CNT>
class Palette: public PaletteBase {
	public:
		Palette(Palette<CNT>&& other) noexcept {
			mColors = std::move(other.mColors);
		}

		constexpr Palette(const std::array<uint8_t, CNT*4>& a) {
			size_t ii = 0;
			for(size_t i = 0; i < CNT; ++i) {
				mColors[i] = (static_cast<uint32_t>(a[ii++])) | (static_cast<uint32_t>(a[ii++]) << 8) | (static_cast<uint32_t>(a[ii++]) << 16) | (static_cast<uint32_t>(a[ii++]) << 24);
			}
		}

		constexpr Palette(const std::array<std::array<uint8_t, 4>, CNT>& a) {
			for(size_t i = 0; i < CNT; ++i) {
				mColors[i] = std::bit_cast<uint32_t>(a);
			}
		}

		void operator=(Palette<CNT>&& other) noexcept {
			if (this != &other) {
				mColors = std::move(other.mColors);
			}
		}

		virtual const std::array<Color, CNT>& getColors() const { return mColors; }

		Color getColor(uint16_t index) const {
			static_assert(isPowerOfTwo(CNT));
			static constexpr auto sMask = CNT - 1;
			return mColors[index & sMask];
		}

		Color getNESColor(uint8_t index) const {
			static_assert(CNT == (64), "NES palette should have 64 colors!");
			return mColors[index & 0x3F];
		}

		const uint32_t* data() const { return mColors.data(); }

	private:
		Palette() {};

	private:
		std::array<Color, CNT> mColors;
};

Palette<64> createNESPalette(const std::array<uint8_t, 192>& a);

bool loadNESPalette(const std::string& filename, Palette<64>& palette);

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PALETTE_H