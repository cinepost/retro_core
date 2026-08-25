#ifndef __RETRO_CORE_FRAMEWORK_COLOR_H
#define __RETRO_CORE_FRAMEWORK_COLOR_H

#include <cstdint>
#include <cassert>
#include <bit>
#include <iomanip>


namespace RetroCore {

union RGBA8888 {
    // Access the entire pixel as a single 32-bit unsigned integer
    uint32_t v;

    // Access individual channels based on system architecture
    struct {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
#else // Defaulting to Little-Endian (x86_64, modern ARM, etc.)
        uint8_t a;
        uint8_t b;
        uint8_t g;
        uint8_t r;
#endif
    };

    inline uint32_t asXRGB8888() const {
        return v >> 8;
    }

    inline operator uint32_t() const { 
        return v; 
    }

	// Right-hand side mask: Pixel & 0xFFFFFF00
    inline uint32_t operator&(uint32_t mask) const {
        return v & mask;
    }

    // Left-hand side mask: 0xFFFFFF00 & Pixel
    inline friend uint32_t operator&(uint32_t mask, const RGBA8888& pixel) {
        return mask & pixel.v;
    }

    // Right Shift Operator: Color >> Bits
    inline uint32_t operator>>(int shift) const {
        return v >> shift;
    }

    RGBA8888() = default;

    constexpr RGBA8888(uint32_t _v): v(_v) {}
    constexpr RGBA8888(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b), a(255) {}
    constexpr RGBA8888(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a): r(_r), g(_g), b(_b), a(_a) {}

};

union RGB111 {
    // Access the entire packed color as a single 8-bit integer
    uint8_t v;

    // Access individual 1-bit color channels
    struct {
        uint8_t unused : 5; // 5 bits (0-4) 
        uint8_t b      : 1; // 1 bit (5)
        uint8_t g      : 1; // 1 bit (6)
        uint8_t r      : 1; // 1 bit (7)
    };

    inline operator uint8_t() const { 
        return v; 
    }

    // Right-hand side bitwise AND: Color & Mask
    inline uint8_t operator&(uint8_t mask) const {
        return v & mask;
    }

    // Left-hand side bitwise AND: Mask & Color
    inline friend uint8_t operator&(uint8_t mask, const RGB111& color) {
        return mask & color.v;
    }

    // Right Shift Operator: Color >> Bits
    inline uint8_t operator>>(int shift) const {
        return v >> shift;
    }

    RGB111() = default;

    constexpr RGB111(uint8_t _v):v(_v) {}
    constexpr RGB111(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b) {}

};

union RGB222 {
    // Access the entire packed color as a single 8-bit integer
    uint8_t v;

    // Access individual 2-bit color channels
    struct {
        uint8_t unused : 2; // Lowest 3 bits (0-1)
        uint8_t b      : 2; // Middle 3 bits (2-3)
        uint8_t g      : 2; // High 3 bits (4-5)
        uint8_t r      : 2; // Remainder bits at the top (6-7)
    };

    inline operator uint8_t() const { 
        return v; 
    }

    // Right-hand side bitwise AND: Color & Mask
    inline uint8_t operator&(uint8_t mask) const {
        return v & mask;
    }

    // Left-hand side bitwise AND: Mask & Color
    inline friend uint8_t operator&(uint8_t mask, const RGB222& color) {
        return mask & color.v;
    }

    // Right Shift Operator: Color >> Bits
    inline uint8_t operator>>(int shift) const {
        return v >> shift;
    }

    RGB222() = default;

    constexpr RGB222(uint8_t _v):v(_v) {}
    constexpr RGB222(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b) {}

};

union RGB333 {
    // Access the entire packed 9-bit color as a single 16-bit integer
    uint16_t v;

    // Access individual 3-bit color channels
    struct {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        uint16_t unused : 7; // Remainder bits at the top
        uint16_t r      : 3; // 3 bits for Red
        uint16_t g      : 3; // 3 bits for Green
        uint16_t b      : 3; // 3 bits for Blue
#else // Defaulting to Little-Endian (x86_64, modern ARM, etc.)
        uint16_t b      : 3; // Lowest 3 bits (0-2)
        uint16_t g      : 3; // Middle 3 bits (3-5)
        uint16_t r      : 3; // High 3 bits (6-8)
        uint16_t unused : 7; // Remainder bits at the top (9-15)
#endif
    };

    inline operator uint16_t() const { 
        return v; 
    }

    // Right-hand side bitwise AND: Color & Mask
    inline uint16_t operator&(uint16_t mask) const {
        return v & mask;
    }

    // Left-hand side bitwise AND: Mask & Color
    inline friend uint16_t operator&(uint16_t mask, const RGB333& color) {
        return mask & color.v;
    }

    // Right Shift Operator: Color >> Bits
    inline uint16_t operator>>(int shift) const {
        return v >> shift;
    }

	RGB333() = default;

    constexpr RGB333(uint16_t _v):v(_v) {}
    constexpr RGB333(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b) {}

};

union GRB332 {
    // Access the entire packed color as a single 8-bit integer
    uint8_t v;

    // Access individual 3-bit color channels
    struct {
        uint8_t g      : 3; // Lowest 3 bits (0-2)
        uint8_t r      : 3; // Middle 3 bits (3-5)
        uint8_t b      : 1; // High 2 bits (6-7)
    };

    inline operator uint8_t() const { 
        return v; 
    }

    // Right-hand side bitwise AND: Color & Mask
    inline uint8_t operator&(uint8_t mask) const {
        return v & mask;
    }

    // Left-hand side bitwise AND: Mask & Color
    inline friend uint8_t operator&(uint8_t mask, const GRB332& color) {
        return mask & color.v;
    }

    // Right Shift Operator: Color >> Bits
    inline uint8_t operator>>(int shift) const {
        return v >> shift;
    }

    GRB332() = default;

    constexpr GRB332(uint8_t _v):v(_v) {}
    constexpr GRB332(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b) {}
};

union RGB233 {
    // Access the entire packed color as a single 8-bit integer
    uint8_t v;

    // Access individual 3-bit color channels
    struct {
        uint8_t r      : 2; // Lowest 2 bits (0-1)
        uint8_t g      : 3; // Middle 3 bits (2-4)
        uint8_t b      : 3; // High 3 bits (5-7)
    };

    inline operator uint8_t() const { 
        return v; 
    }

    // Right-hand side bitwise AND: Color & Mask
    inline uint8_t operator&(uint8_t mask) const {
        return v & mask;
    }

    // Left-hand side bitwise AND: Mask & Color
    inline friend uint8_t operator&(uint8_t mask, const RGB233& color) {
        return mask & color.v;
    }

    // Right Shift Operator: Color >> Bits
    inline uint8_t operator>>(int shift) const {
        return v >> shift;
    }

    RGB233() = default;

    constexpr RGB233(uint8_t _v):v(_v) {}
    constexpr RGB233(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b) {}
};

inline std::string to_hex_string(const RetroCore::RGBA8888& c) {
    uint32_t r = (c.v >> 24) & 0x000000FF;
    uint32_t g = (c.v >> 16) & 0x000000FF;
    uint32_t b = (c.v >> 8)  & 0x000000FF;
    uint32_t a = c.v & 0x000000FF;

    std::stringstream ss;
    ss << "#" 
       << std::hex << std::setfill('0') 
       << std::setw(2) << r
       << std::setw(2) << g
       << std::setw(2) << b
       << std::setw(2) << a;
       
    return ss.str();
}

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PALETTE_H