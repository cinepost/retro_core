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
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
        #pragma clang diagnostic ignored "-Wnested-anon-types"
    #elif defined(__GNUC__) // True for GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic" 
    #endif

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

    #if defined(__clang__)
        #pragma clang diagnostic pop
    #elif defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif

    inline uint32_t asXRGB8888() const {
        return v >> 8;
    }

    constexpr operator uint32_t() const noexcept { 
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
    constexpr uint32_t operator>>(int shift) const noexcept {
        return v >> shift;
    }

    RGBA8888() = default;

    constexpr RGBA8888(uint32_t _v): v(_v) {}

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    constexpr RGBA8888(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b), a(255) {}
    constexpr RGBA8888(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a): r(_r), g(_g), b(_b), a(_a) {}
#else // Defaulting to Little-Endian (x86_64, modern ARM, etc.)
    constexpr RGBA8888(uint8_t _r, uint8_t _g, uint8_t _b): a(255), b(_b), g(_g), r(_r) {}
    constexpr RGBA8888(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a): a(_a), b(_b), g(_g), r(_r) {}
#endif

};

union RGB111 {
    // Access the entire packed color as a single 8-bit integer
    uint8_t v;

    // Access individual 1-bit color channels
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
        #pragma clang diagnostic ignored "-Wnested-anon-types"
    #elif defined(__GNUC__) // True for GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic" 
    #endif

    struct {
        uint8_t unused : 5; // 5 bits (0-4) 
        uint8_t b      : 1; // 1 bit (5)
        uint8_t g      : 1; // 1 bit (6)
        uint8_t r      : 1; // 1 bit (7)
    };

    #if defined(__clang__)
        #pragma clang diagnostic pop
    #elif defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif

    constexpr operator uint8_t() const noexcept { 
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
    constexpr uint8_t operator>>(int shift) const noexcept {
        return v >> shift;
    }

    RGB111() = default;

    constexpr RGB111(uint8_t _v):v(_v) {}
    constexpr RGB111(uint8_t _r, uint8_t _g, uint8_t _b) 
        : b(static_cast<uint8_t>(_b & 0x01)) // Mask to 1 bits (max value 1)
        , g(static_cast<uint8_t>(_g & 0x01)) // Mask to 1 bits (max value 1)
        , r(static_cast<uint8_t>(_r & 0x01)) // Mask to 1 bits (max value 1)
    {}
};

union RGB222 {
    // Access the entire packed color as a single 8-bit integer
    uint8_t v;

    // Access individual 2-bit color channels
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
        #pragma clang diagnostic ignored "-Wnested-anon-types"
    #elif defined(__GNUC__) // True for GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic" 
    #endif

    struct {
        uint8_t unused : 2; // Lowest 3 bits (0-1)
        uint8_t b      : 2; // Middle 3 bits (2-3)
        uint8_t g      : 2; // High 3 bits (4-5)
        uint8_t r      : 2; // Remainder bits at the top (6-7)
    };

    #if defined(__clang__)
        #pragma clang diagnostic pop
    #elif defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif

    constexpr operator uint8_t() const noexcept { 
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
    constexpr uint8_t operator>>(int shift) const noexcept {
        return v >> shift;
    }

    RGB222() = default;

    constexpr RGB222(uint8_t _v):v(_v) {}
    constexpr RGB222(uint8_t _r, uint8_t _g, uint8_t _b) 
        : b(static_cast<uint8_t>(_b & 0x03)) // Mask to 2 bits (max value 3)
        , g(static_cast<uint8_t>(_g & 0x03)) // Mask to 2 bits (max value 3)
        , r(static_cast<uint8_t>(_r & 0x03)) //Mask to 2 bits (max value 3)
    {}
};

union RGB333 {
    // Access the entire packed 9-bit color as a single 16-bit integer
    uint16_t v;

    // Access individual 3-bit color channels
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
        #pragma clang diagnostic ignored "-Wnested-anon-types"
    #elif defined(__GNUC__) // True for GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic" 
    #endif

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

    #if defined(__clang__)
        #pragma clang diagnostic pop
    #elif defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif

    constexpr operator uint16_t() const noexcept { 
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
    constexpr uint16_t operator>>(int shift) const noexcept {
        return v >> shift;
    }

	RGB333() = default;

    constexpr RGB333(uint16_t _v):v(_v) {}
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    //constexpr RGB333(uint8_t _r, uint8_t _g, uint8_t _b): r(_r), g(_g), b(_b) {}
    constexpr RGB333(uint8_t _r, uint8_t _g, uint8_t _b) 
        : r(static_cast<uint16_t>(_r & 0x0007)) // Mask to 3 bits (max value 7)
        , g(static_cast<uint16_t>(_g & 0x0007)) // Mask to 3 bits (max value 7)
        , b(static_cast<uint16_t>(_b & 0x0007)) // Mask to 3 bits (max value 7)
    {}
#else // Defaulting to Little-Endian (x86_64, modern ARM, etc.)
    //constexpr RGB333(uint8_t _r, uint8_t _g, uint8_t _b): b(_b), g(_g), r(_r) {}
    constexpr RGB333(uint8_t _r, uint8_t _g, uint8_t _b) 
        : b(static_cast<uint16_t>(_b & 0x0007)) // Mask to 3 bits (max value 7)
        , g(static_cast<uint16_t>(_g & 0x0007)) // Mask to 3 bits (max value 7)
        , r(static_cast<uint16_t>(_r & 0x0007)) // Mask to 3 bits (max value 7)
    {}
#endif
};

union GRB332 {
    // Access the entire packed color as a single 8-bit integer
    uint8_t v;

    // Access individual 3-bit color channels
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
        #pragma clang diagnostic ignored "-Wnested-anon-types"
    #elif defined(__GNUC__) // True for GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic" 
    #endif

    struct {
        uint8_t g      : 3; // Lowest 3 bits (0-2)
        uint8_t r      : 3; // Middle 3 bits (3-5)
        uint8_t b      : 2; // High 2 bits (6-7)
    };

    #if defined(__clang__)
        #pragma clang diagnostic pop
    #elif defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif

    constexpr operator uint8_t() const noexcept { 
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
    constexpr uint8_t operator>>(int shift) const noexcept {
        return v >> shift;
    }

    GRB332() = default;

    constexpr GRB332(uint8_t _v):v(_v) {}
    constexpr GRB332(uint8_t _r, uint8_t _g, uint8_t _b) 
        : g(static_cast<uint8_t>(_g & 0x07)) // Mask to 3 bits (max value 7)
        , r(static_cast<uint8_t>(_r & 0x07)) // Mask to 3 bits (max value 7)
        , b(static_cast<uint8_t>(_b & 0x03)) // Mask to 2 bits (max value 3)
    {}
};

union RGB233 {
    // Access the entire packed color as a single 8-bit integer
    uint8_t v;

    // Access individual 3-bit color channels
    #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
        #pragma clang diagnostic ignored "-Wnested-anon-types"
    #elif defined(__GNUC__) // True for GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic" 
    #endif

    struct {
        uint8_t r      : 2; // Lowest 2 bits (0-1)
        uint8_t g      : 3; // Middle 3 bits (2-4)
        uint8_t b      : 3; // High 3 bits (5-7)
    };

    #if defined(__clang__)
        #pragma clang diagnostic pop
    #elif defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif

    constexpr operator uint8_t() const noexcept { 
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
    constexpr uint8_t operator>>(int shift) const noexcept {
        return v >> shift;
    }

    RGB233() = default;

    constexpr RGB233(uint8_t _v):v(_v) {}
    constexpr RGB233(uint8_t _r, uint8_t _g, uint8_t _b) 
        : r(static_cast<uint8_t>(_r & 0x03)) // Mask to 2 bits (max value 3)
        , g(static_cast<uint8_t>(_g & 0x07)) // Mask to 3 bits (max value 7)
        , b(static_cast<uint8_t>(_b & 0x07)) // Mask to 3 bits (max value 7)
    {}
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