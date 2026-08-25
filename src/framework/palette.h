#ifndef __RETRO_CORE_FRAMEWORK_PALETTE_H
#define __RETRO_CORE_FRAMEWORK_PALETTE_H

#include "framework/formats.h"
#include "framework/color.h"
#include "framework/color_utils.h"

#include <cstdint>
#include <cassert>
#include <array>
#include <iostream>
#include <fstream>
#include <bit>
#include <limits>

namespace RetroCore {

struct PaletteBase {
	using Color = RGBA8888;
};

template<uint32_t CNT>
class Palette: public PaletteBase {
	public:
		Palette(Palette<CNT>&& other) noexcept {
			mColors = std::move(other.mColors);
		}

		constexpr Palette(const std::array<uint8_t, CNT*4>& a) {
			size_t ii = 0;
			for(uint32_t i = 0; i < CNT; ++i) {
				mColors[i].r = a[ii++]; mColors[i].g = a[ii++]; mColors[i].b = a[ii++]; mColors[i].a = a[ii++];
			}
		}

		constexpr Palette(const std::array<std::array<uint8_t, 4>, CNT>& a) {
			for(uint32_t i = 0; i < CNT; ++i) {
				mColors[i] = std::bit_cast<uint32_t>(a[i]);
			}
		}

		void operator=(const Palette<CNT>& other) noexcept {
			if (this != &other) {
				mColors = other.mColors;
			}
		}

		void operator=(Palette<CNT>&& other) noexcept {
			if (this != &other) {
				mColors = std::move(other.mColors);
			}
		}

		// Array access operators
		[[nodiscard]] constexpr Color operator[](uint32_t index) const noexcept {
			if constexpr(isPowerOfTwo(CNT)) {
				return mColors[index & sIndexMask];
			} else {
				assert(index < mColors.size());
				return mColors[index];
			}
		}

		[[nodiscard]] constexpr Color& operator[](uint32_t index) noexcept {
			if constexpr(isPowerOfTwo(CNT)) {
				return mColors[index & sIndexMask];
			} else {
				assert(index < mColors.size());
				return mColors[index];
			}
		}

		uint32_t findClosestColorIndex(const Color& color, bool includeAlpha = false) const {
			    uint32_t closestIndex = 0;
			    uint32_t minDistanceSq = std::numeric_limits<uint32_t>::max();

			    for (uint32_t i = 0; i < CNT; ++i) {
			        // Calculate channel deltas using cast to prevent unsigned underflow
			        int dr = static_cast<int>(color.r) - static_cast<int>(mColors[i].r);
			        int dg = static_cast<int>(color.g) - static_cast<int>(mColors[i].g);
			        int db = static_cast<int>(color.b) - static_cast<int>(mColors[i].b);
			        
			        // Sum of squares (Euclidean distance squared)
			        uint32_t distanceSq = (dr * dr) + (dg * dg) + (db * db);

			        // Optional Alpha channel comparison
			        if (includeAlpha) {
			            int da = static_cast<int>(color.a) - static_cast<int>(mColors[i].a);
			            distanceSq += (da * da);
			        }

			        // Keep track of the minimum distance match
			        if (distanceSq < minDistanceSq) {
			            minDistanceSq = distanceSq;
			            closestIndex = i;
			        }
			    }

			    return closestIndex;
		}

		virtual const std::array<Color, CNT>& getColors() const { return mColors; }

		[[nodiscard]] Color getColor(uint32_t index) const {
			return (*this)[index];
		}

		[[nodiscard]] Color getNESColor(uint8_t index) const {
			static_assert(CNT == (64), "NES palette should have 64 colors!");
			return mColors[index & 0x3F];
		}

		void setColor(uint32_t index, const Color c) {
			mColors[index] = c;
		}

		void setColor(uint32_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
			mColors[index].r = r; mColors[index].g = g; mColors[index].b = b; mColors[index].a = a;
		}

		const Color* data() const { return mColors.data(); }

		[[nodiscard]] constexpr size_t size() const { return mColors.size(); }

		Palette() {};

	private:
		static constexpr size_t sIndexMask = static_cast<size_t>(CNT - 1u);
		std::array<Color, CNT> mColors;
};

Palette<64> createNESPalette(const std::array<uint8_t, 192>& a);

bool loadNESPalette(const std::string& filename, Palette<64>& palette);

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PALETTE_H