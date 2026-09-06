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
	MSX,
    RAW
};

struct FramebufferDims { 
	uint16_t width; 
	uint16_t height; 
};

template<typename T>
struct Vec2 {
	T x;
	T y;

	Vec2(): x(0u), y(0u) {}
	Vec2(T _x, T _y): x(_x), y(_y) {}

	bool operator==(const Vec2& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vec2& other) const {
        return !(*this == other);
    }

    Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2& operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    friend Vec2 operator+(Vec2 lhs, const Vec2& rhs) {
        lhs += rhs;
        return lhs;
    }

    friend Vec2 operator-(Vec2 lhs, const Vec2& rhs) {
        lhs -= rhs;
        return lhs;
    }
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_TYPES_H