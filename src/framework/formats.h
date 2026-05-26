#ifndef __RETRO_CORE_FRAMEWORK_FORMATS_H
#define __RETRO_CORE_FRAMEWORK_FORMATS_H

#include "framework/common.h"
#include "framework/types.h"

#include <cstdint>
#include <cassert>


namespace RetroCore {

enum class PixelFormat: uint8_t {
	NONE,
    Indexed1BPP,
    Indexed2BPP,
    Indexed4BPP,
    Indexed6BPP,
    Indexed8BPP,
    RGB111,
    RGB222,
    RGB444,
    RGB565,
    RGB666,
    RGB888,
    RGBA1111,
    RGBA2222,
    RGBA4444,
    RGBA5551,
    RGBA6666,
    RGBA8888
};

[[nodiscard]] constexpr uint8_t bitsPerPixel(PixelFormat format) noexcept {
	switch(format) {
		case PixelFormat::Indexed1BPP:
			return 1;
		case PixelFormat::Indexed2BPP:
			return 2;
		case PixelFormat::Indexed4BPP:
			return 4;
		case PixelFormat::Indexed6BPP:
			return 6;
		case PixelFormat::Indexed8BPP:
			return 8;
		case PixelFormat::RGB111:
			return 3;
		case PixelFormat::RGB222:
			return 6;
		case PixelFormat::RGB444:
			return 12;
		case PixelFormat::RGB565:
			return 16;
		case PixelFormat::RGB666:
			return 18;
		case PixelFormat::RGB888:
			return 24;
		case PixelFormat::RGBA1111:
			return 4;
		case PixelFormat::RGBA2222:
			return 8;
		case PixelFormat::RGBA4444:
		case PixelFormat::RGBA5551:
			return 16;
		case PixelFormat::RGBA6666:
			return 24;
		case PixelFormat::RGBA8888:
			return 32;
		default:
			return 0;
	}
}

[[nodiscard]] constexpr uint8_t bytesPerPixel(PixelFormat format) noexcept {
	return bitsToBytesCount(bitsPerPixel(format));
}

[[nodiscard]] constexpr bool isIndexedColorFormat(const PixelFormat format) noexcept {
	return format <= PixelFormat::Indexed8BPP && format != PixelFormat::NONE;
}

[[nodiscard]] constexpr bool hasAlphaChannel(PixelFormat format) noexcept {
    return format >= PixelFormat::RGBA1111;
}

[[nodiscard]] constexpr PixelFormat getProfileNativeFramebufferPixelFormat(const Platform profile) {
	switch(profile) {
		case Platform::SMS:
			return PixelFormat::RGB222;
		case Platform::NES:
			return PixelFormat::Indexed4BPP;
		default:
			static_assert("Should not be here!");
			return PixelFormat::NONE;
	}
}

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_FORMATS_H