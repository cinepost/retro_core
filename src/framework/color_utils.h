#ifndef __RETRO_CORE_FRAMEWORK_COLOR_UTILS_H
#define __RETRO_CORE_FRAMEWORK_COLOR_UTILS_H

#include "color.h"

namespace RetroCore {

/**
 * Quantizes an 8-bit color channel down to a V9938 3-bit space.
 * Uses accurate rounding (adds 18 before dividing by 36) rather than truncation.
 */
[[ nodiscard ]] inline constexpr uint8_t quantize_8_to_3(uint8_t value8) noexcept {
    // Exact mapping: value3 = round(value8 * 7.0 / 255.0)
    // Integer approximation equivalent: (value8 * 7 + 127) / 255
    return (uint8_t)(((uint16_t)value8 * 7 + 127) / 255);
}

/**
 * Upscales a 3-bit V9938 color channel back to full 8-bit space.
 * Uses exact bit replication (Value * 36 + Value / 2) to perfectly match (Value * 255 / 7)
 */
[[ nodiscard ]] inline constexpr uint8_t upscale_3_to_8(uint8_t value3) noexcept {
    // V9938 hardware mapping: 
    // 0->0, 1->36, 2->73, 3->109, 4->146, 5->182, 6->219, 7->255
    return (uint8_t)((value3 << 5) | (value3 << 2) | (value3 >> 1));
}

/**
 * Packs RGBA8888 into a 9-bit V9938 hardware-compatible bit array.
 */
[[ nodiscard ]] inline constexpr RGB333 rgb888_to_v9938(RGBA8888 color) noexcept {
    uint8_t r3 = quantize_8_to_3(color.r);
    uint8_t g3 = quantize_8_to_3(color.g);
    uint8_t b3 = quantize_8_to_3(color.b);
    
    // Pack into a single 16-bit word: RRGGGBBB
    return (RGB333)((r3 << 6) | (g3 << 3) | b3);
}

/**
 * Unpacks a 9-bit V9938 color back to RGBA8888.
 */
[[ nodiscard ]] inline RGBA8888 v9938_to_rgb888(RGB333 color333) noexcept {
    uint8_t r3 = (uint16_t)(color333 >> 6) & 0x07;
    uint8_t g3 = (uint16_t)(color333 >> 3) & 0x07;
    uint8_t b3 = (uint16_t)color333 & 0x07;
    
    RGBA8888 color888;
    color888.r = upscale_3_to_8(r3);
    color888.g = upscale_3_to_8(g3);
    color888.b = upscale_3_to_8(b3);
    
    return color888;
}


[[ nodiscard ]] inline constexpr uint32_t rgb333_to_rgba8888(uint16_t rgb333) noexcept {
    // Extract channels
    uint32_t r3 = (rgb333 >> 6) & 0x07;
    uint32_t g3 = (rgb333 >> 3) & 0x07;
    uint32_t b3 = rgb333        & 0x07;

    // Scale 3-bit to 8-bit using bit replication: (val << 5) | (val << 2) | (val >> 1)
    uint32_t r8 = (r3 << 5) | (r3 << 2) | (r3 >> 1);
    uint32_t g8 = (g3 << 5) | (g3 << 2) | (g3 >> 1);
    uint32_t b8 = (b3 << 5) | (b3 << 2) | (b3 >> 1);
    uint32_t a8 = 0xFF; // Fully opaque alpha channel

    // Pack into RGBA8888 (0xRRGGBBAA layout)
    return (r8 << 24) | (g8 << 16) | (b8 << 8) | a8;
}

[[ nodiscard ]] inline constexpr uint16_t rgb888_to_rgb333_fast(uint32_t rgb888) noexcept {
    return ((rgb888 >> 10) & 0x01C0) |  // Red: Shift right 16, downscale 5, shift up 6 (16-5-6 = 10)
           ((rgb888 >> 7)  & 0x0038) |  // Green: Shift right 8, downscale 5, shift up 3 (8-5-3 = 7)
           ((rgb888 >> 5)  & 0x0007);   // Blue: Shift right 0, downscale 5 (5)
}

[[ nodiscard ]] inline constexpr uint32_t convert_RGB233_to_RGBA8888(uint8_t grb332) noexcept {
    // Extract raw components using bit shifting and masking
    uint8_t rawG = (grb332 >> 5) & 0x07; // Top 3 bits
    uint8_t rawR = (grb332 >> 2) & 0x07; // Middle 3 bits
    uint8_t rawB =  grb332       & 0x03; // Bottom 2 bits

    // Bit-replication scaling to map values cleanly to 0-255 range
    uint8_t r = (rawR << 5) | (rawR << 2) | (rawR >> 1); 
    uint8_t g = (rawG << 5) | (rawG << 2) | (rawG >> 1); 
    uint8_t b = (rawB << 6) | (rawB << 4) | (rawB << 2) | rawB;
    uint8_t a = 0xFF; // Full opacity

    // Pack into 32-bit RGBA format (0xRRGGBBAA)
    return (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(b) << 8) | static_cast<uint32_t>(a);
}

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_PALETTE_H