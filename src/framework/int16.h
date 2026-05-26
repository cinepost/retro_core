#ifndef __RETRO_CORE_FRAMEWORK_INT16_H
#define __RETRO_CORE_FRAMEWORK_INT16_H

#include <cstdint>
#include <cstddef>
#include <iostream>

namespace RetroCore {

// Aligned to 4 bytes (2 components * 2 bytes) to optimize cache and register loads
struct alignas(4) uint16_t2 {
    union {
        struct {
            std::uint16_t x;
            std::uint16_t y;
        };
        struct {
            std::uint16_t u;
            std::uint16_t v;
        };
        struct {
            std::uint16_t width;
            std::uint16_t height;
        };
        std::uint16_t data[2];
    };

    // Constructors
    constexpr uint16_t2() noexcept : x(0), y(0) {}
    constexpr uint16_t2(std::uint16_t _x, std::uint16_t _y) noexcept : x(_x), y(_y) {}
    explicit constexpr uint16_t2(std::uint16_t scalar) noexcept : x(scalar), y(scalar) {}
    constexpr uint16_t2(const uint16_t2& scalar) noexcept : x(scalar.x), y(scalar.y) {}

    // Array access operators
    [[nodiscard]] constexpr std::uint16_t operator[](std::size_t index) const noexcept {
        return data[index];
    }

    [[nodiscard]] constexpr std::uint16_t& operator[](std::size_t index) noexcept {
        return data[index];
    }

    // Unsigned arithmetic operators
    constexpr uint16_t2& operator+=(const uint16_t2& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr uint16_t2& operator-=(const uint16_t2& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    // Equality operators
    [[nodiscard]] constexpr bool operator==(const uint16_t2& rhs) const noexcept {
        return x == rhs.x && y == rhs.y;
    }

    [[nodiscard]] constexpr bool operator!=(const uint16_t2& rhs) const noexcept {
        return !(*this == rhs);
    }
};

// Binary operations
[[nodiscard]] constexpr uint16_t2 operator+(uint16_t2 lhs, const uint16_t2& rhs) noexcept {
    lhs += rhs;
    return lhs;
}

[[nodiscard]] constexpr uint16_t2 operator-(uint16_t2 lhs, const uint16_t2& rhs) noexcept {
    lhs -= rhs;
    return lhs;
}


}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_INT16_H