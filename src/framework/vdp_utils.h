#ifndef __RETRO_CORE_FRAMEWORK_VDP_UTILS_H
#define __RETRO_CORE_FRAMEWORK_VDP_UTILS_H

#include "framework/common.h"
#include "palette.h"
#include "framework/types.h"
#include "framework/formats.h"

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>
#include <limits>


namespace RetroCore {

static constexpr size_t sUint16Max = std::numeric_limits<uint16_t>::max();

[[nodiscard]] constexpr size_t nativeFramebufferStride(const Platform profile, const uint16_t width) {
	assert(width > 0 && width <= sUint16Max);
	size_t bytes_per_line = static_cast<size_t>(bytesPerPixel(getProfileNativeFramebufferPixelFormat(profile))) * width;
	assert(bytes_per_line <= sUint16Max);
	return bytes_per_line;
}

[[nodiscard]] constexpr size_t nativeFramebufferSize(const Platform profile, const uint16_t width, const uint16_t height) {
	assert(height > 0 && height <= sUint16Max);
	return nativeFramebufferStride(profile, width) * height;
}

[[nodiscard]] constexpr uint32_t getCRAMLineSize(const Platform profile) {
	switch(profile) {
		case Platform::SMS:
			// The SMS has a total CRAM capacity of 32 colors, divided cleanly into two 16-color palette lines. 
			// Line 0 is reserved exclusively for background tiles. Line 1 (CRAM entries 16–31) is reserved exclusively for sprites.
			return 16; 
		case Platform::NES:
			// Slot 0 of each individual sub-palette mirrors back to the global backdrop color (or functions as transparency for sprites), keeping the hard limit at 25
			return 4; 
		default:
			static_assert("Should not be here!");
	}
}

[[nodiscard]] constexpr uint32_t getCRAMLinesCount(const Platform profile) {
	switch(profile) {
		case Platform::SMS:
			return 2; // Two 16-color palette lines
		case Platform::NES:
			return 8; // 4 sub-palettes for sprites and 4 sub-palettes for background
		default:
			static_assert("Should not be here!");
	} 
}

[[nodiscard]] constexpr uint8_t getCRAMLineIndexMask(const Platform profile) {
	uint8_t mask = 0;
	uint32_t value = getCRAMLinesCount(profile);
    while (value) {
        mask += (value & 1);
        value >>= 1;
    }
    return mask;
}

template <typename M, typename T>
[[nodiscard]] constexpr M getIndexMask(T index) {
	M mask = 0;
	uint32_t value = (uint32_t)(index);
    while (value) {
        mask += (value & 1);
        value >>= 1;
    }
    return mask;
}

// Translates internal native bits (e.g., 3-bit RGB) to 32-bit RGBA for screen output
template <Platform VDP>
uint32_t nativeToRGBA8888(uint16_t nativeColor) {
	if constexpr (VDP == Platform::MD) { 
		// Sega Megadrive
		uint8_t r = ((nativeColor & 0x007) >> 0) * 36;
		uint8_t g = ((nativeColor & 0x038) >> 3) * 36;
		uint8_t b = ((nativeColor & 0x1C0) >> 6) * 36;
		return (r << 24) | (g << 16) | (b << 8) | 0xFF;
	} else if constexpr (VDP == Platform::SNES) { 
		// SNES
		uint8_t r = ((nativeColor & 0x001F) >> 0) * 8;
		uint8_t g = ((nativeColor & 0x03E0) >> 5) * 8;
		uint8_t b = ((nativeColor & 0x7C00) >> 10) * 8;
		return (r << 24) | (g << 16) | (b << 8) | 0xFF;
	} else {
		static_assert("Unimplemented");
	}
}

template <Platform VDP>
uint32_t nativeToRGBA8888(uint8_t nativeColor) {
	if constexpr (VDP == Platform::NES) { 
		// NES
		// nativeColor here is an index into NTSC/PAL to RGB palette
		static const Palette<64> sPalette = createNESPalette({ 
			0x6b, 0x6b, 0x6b, 0x00, 0x1b, 0x87, 0x21, 0x00, 0x9a, 0x40, 0x00, 0x8c,
			0x60, 0x00, 0x67, 0x64, 0x00, 0x1e, 0x59, 0x08, 0x00, 0x46, 0x16, 0x00,
			0x26, 0x36, 0x00, 0x00, 0x45, 0x00, 0x00, 0x47, 0x08, 0x00, 0x42, 0x1d,
			0x00, 0x36, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xb4, 0xb4, 0xb4, 0x15, 0x55, 0xce, 0x43, 0x37, 0xea, 0x71, 0x24, 0xda,
			0x9c, 0x1a, 0xb6, 0xaa, 0x11, 0x64, 0xa8, 0x2e, 0x00, 0x87, 0x4b, 0x00,
			0x66, 0x6b, 0x00, 0x21, 0x83, 0x00, 0x00, 0x8a, 0x00, 0x00, 0x81, 0x44,
			0x00, 0x76, 0x91, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xff, 0xff, 0xff, 0x63, 0xaf, 0xff, 0x82, 0x96, 0xff, 0xc0, 0x7d, 0xfe,
			0xe9, 0x77, 0xff, 0xf5, 0x72, 0xcd, 0xf4, 0x88, 0x6b, 0xdd, 0xa0, 0x29,
			0xbd, 0xbd, 0x0a, 0x89, 0xd2, 0x0e, 0x5c, 0xde, 0x3e, 0x4b, 0xd8, 0x86,
			0x4d, 0xcf, 0xd2, 0x50, 0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xff, 0xff, 0xff, 0xbe, 0xe1, 0xff, 0xd4, 0xd4, 0xff, 0xe3, 0xca, 0xff,
			0xf0, 0xc9, 0xff, 0xff, 0xc6, 0xe3, 0xff, 0xce, 0xc9, 0xf4, 0xdc, 0xaf,
			0xeb, 0xe5, 0xa1, 0xd2, 0xef, 0xa2, 0xbe, 0xf4, 0xb5, 0xb8, 0xf1, 0xd0,
			0xb8, 0xed, 0xf1, 0xbd, 0xbd, 0xbd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
		});

		uint32_t value;
		std::memcpy(&value, sPalette.getNESColor(nativeColor).data(), 4);
		return value;
	} else if constexpr (VDP == Platform::SMS) {
		// SMS (6-bit RGB)
		// Hardware translates 2-bit values (0-3) to 8-bit color intensities
		// Steps: 0 -> 0,  1 -> 85,  2 -> 170,  3 -> 255 (Value * 85)
		uint8_t r = ((nativeColor & 0x03) >> 0) * 85;
		uint8_t g = ((nativeColor & 0x0C) >> 2) * 85;
		uint8_t b = ((nativeColor & 0x30) >> 4) * 85;
		return (r << 24) | (g << 16) | (b << 8) | 0xFF;
	} else {
		static_assert("Unimplemented");
	}
}

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_VDP_UTILS_H