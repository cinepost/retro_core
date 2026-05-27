#ifndef __RETRO_CORE_FRAMEWORK_TYPES_H
#define __RETRO_CORE_FRAMEWORK_TYPES_H

#include "framework/int16.h"

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>

namespace RetroCore {

enum class Platform: uint8_t {
	NES,
	SNES,
	SMS,
	MD,
	TG16,
	MSX
};

struct FramebufferDims { 
	uint16_t width; 
	uint16_t height; 
};

struct Coord {
	uint32_t x;
	uint32_t y;

	Coord(): x(0u), y(0u) {}
	Coord(uint32_t _x, uint32_t _y): x(_x), y(_y) {}
};

struct CoordRel {
	int x;
	int y;

	CoordRel(): x(0), y(0) {}
	CoordRel(int _x, int _y): x(_x), y(_y) {}
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_TYPES_H