#ifndef _RETRO_CORE_FRAMEWORK_STATIC_DATA_H
#define _RETRO_CORE_FRAMEWORK_STATIC_DATA_H

#include <cstdint>
#include <cstring>
#include <array>
#include <cassert>


namespace RetroCore {

// 8x8 XRGB Crosshair
// 0x00FFFFFF = White Pixel
// 0x00000000 = "Transparent" (or Black)
namespace DebugData {

    enum class CursorType { 
        CROSSHAIR,
        HAND,
        POINTER 
    };
    
    constexpr uint32_t W = 0x00FFFFFF; // 0x00FFFFFF = White Pixel
    constexpr uint32_t _ = 0x00000000; // 0x00000000 = "Transparent" (or Black)

    template <uint16_t W, uint16_t H, uint8_t BPP>
    struct Cursor {
        static constexpr uint16_t width   = W;
        static constexpr uint16_t height  = H;
        static constexpr uint8_t  bpp     = BPP; // 2:RGB16, 3:RGB, 4:RGBA
        uint16_t        point_x;
        uint16_t        point_y;
        uint8_t         pixel_data[W * H * BPP];
        constexpr Cursor(uint16_t _point_x, uint16_t _point_y, const std::array<uint32_t, W * H>& data): point_x(_point_x), point_y(_point_y), pixel_data{} { 
            assert(BPP == sizeof(uint32_t)); 
            for (size_t i = 0; i < (W * H); ++i) {
                pixel_data[i * 4 + 0] = (data[i] >> 0) & 0xFF;
                pixel_data[i * 4 + 1] = (data[i] >> 8) & 0xFF;
                pixel_data[i * 4 + 2] = (data[i] >> 16)  & 0xFF;
                pixel_data[i * 4 + 3] = (data[i] >> 24)  & 0xFF;
            }
        }

        constexpr Cursor(uint16_t _point_x, uint16_t _point_y, const char(& data)[W * H * BPP + 1]): point_x(_point_x), point_y(_point_y), pixel_data{} { 
            assert(BPP == sizeof(uint32_t)); 
            for (size_t i = 0; i < W * H * BPP; ++i) {
                pixel_data[i] = static_cast<uint8_t>(data[i]);
            }
        }

        const uint8_t* data() const { return pixel_data; }
    };

    inline constexpr Cursor<7, 7, 4> Crosshair = {3, 3, std::array<uint32_t, 49>({
            _, _, _, W, _, _, _,
            _, _, _, W, _, _, _,
            _, _, _, W, _, _, _,
            W, W, W, _, W, W, W,
            _, _, _, W, _, _, _,
            _, _, _, W, _, _, _,
            _, _, _, W, _, _, _,
        })
    };

    inline constexpr Cursor<15, 16, 4> PointerHand = {4, 1,
        "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\217\021\021\021\357\000\000\000`\000\000\000\000\000\000\000\000\000"
        "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
        "\000\000\020@@@\377\357\357\357\377\040\040\040\377\000\000\000\020\000\000\000\000\000\000\000\000\000\000"
        "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\020@@@"
        "\377\377\377\377\377\217\217\217\377\000\000\000p\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
        "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\060\060\060\377\377"
        "\377\377\377\317\317\317\377\000\000\000\237\000\000\000\060\000\000\000\040\000\000\000\000\000\000\000\000"
        "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\277\357\357"
        "\357\377\377\377\377\377\000\000\000\377@@@\377\060\060\060\377\000\000\000\237\"\"\"\357"
        "\022\022\022\337\000\000\000\060\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\200"
        "\257\257\257\377\377\377\377\377\060\060\060\377\357\357\357\377\357\357\357"
        "\377\000\000\000\377\357\357\357\377\277\277\277\377\000\000\000\337\040\040\040\377\000\000"
        "\000\237\000\000\000\200\060\060\060\377\022\022\022\337\000\000\000pppp\377\377\377\377\377"
        "PPP\377\357\357\357\377\377\377\377\377PPP\377\377\377\377\377\377\377\377"
        "\377\060\060\060\377\377\377\377\377\060\060\060\377\040\040\040\377\377\377\377\377"
        "\337\337\337\377\"\"\"\357@@@\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377@@@\377\000\000\000\277\317\317\317\377\377\377"
        "\377\377\317\317\317\377\040\040\040\377\377\377\377\377\377\377\377\377\337"
        "\337\337\377\377\377\377\377\337\337\337\377\377\377\377\377\337\337\337"
        "\377\377\377\377\377\377\377\377\377@@@\377\000\000\000\060\021\021\021\357\357\357"
        "\357\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "@@@\377\377\377\377\377@@@\377\377\377\377\377@@@\377\377\377\377\377\377"
        "\377\377\377\"\"\"\357\000\000\000\000\000\000\000PPPP\377\377\377\377\377\377\377\377"
        "\377\377\377\377\377\377\377\377\377@@@\377\377\377\377\377@@@\377\377\377"
        "\377\377@@@\377\377\377\377\377\337\337\337\377\000\000\000\277\000\000\000\000\000\000\000\000"
        "\000\000\000\257\277\277\277\377\377\377\377\377\377\377\377\377\377\377\377\377"
        "@@@\377\377\377\377\377@@@\377\377\377\377\377@@@\377\377\377\377\377PPP"
        "\377\000\000\000`\000\000\000\000\000\000\000\000\000\000\000\040\021\021\021\357\317\317\317\377\377\377"
        "\377\377\377\377\377\377@@@\377\377\377\377\377PPP\377\377\377\377\377PP"
        "P\377\257\257\257\377\000\000\000\257\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\060"
        "\021\021\021\357\317\317\317\377\377\377\377\377\377\377\377\377\377\377\377"
        "\377\357\357\357\377\377\377\377\377\377\377\377\377\"\"\"\357\000\000\000\020\000"
        "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\060\060\060\060\377\377\377\377"
        "\377\377\377\377\377\357\357\357\377\063\063\063\357\060\060\060\377\357\357\357"
        "\377\000\000\000\277\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
        "\000\000\337@@@\377@@@\377\"\"\"\357\000\000\000`\000\000\000`\040\040\040\377\000\000\000\337\000\000"
        "\000\000\000\000\000\000"
    };

}  // namespace DebugData


namespace Palettes {

// NES PAL palette.
constexpr std::array<uint8_t, 256> kNesDefaultPalette { 
    0x6b, 0x6b, 0x6b, 0xff,   0x00, 0x1b, 0x87, 0xff,   0x21, 0x00, 0x9a, 0xff,   0x40, 0x00, 0x8c, 0xff, 
    0x60, 0x00, 0x67, 0xff,   0x64, 0x00, 0x1e, 0xff,   0x59, 0x08, 0x00, 0xff,   0x46, 0x16, 0x00, 0xff, 
    0x26, 0x36, 0x00, 0xff,   0x00, 0x45, 0x00, 0xff,   0x00, 0x47, 0x08, 0xff,   0x00, 0x42, 0x1d, 0xff, 
    0x00, 0x36, 0x59, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff, 
    0xb4, 0xb4, 0xb4, 0xff,   0x15, 0x55, 0xce, 0xff,   0x43, 0x37, 0xea, 0xff,   0x71, 0x24, 0xda, 0xff, 
    0x9c, 0x1a, 0xb6, 0xff,   0xaa, 0x11, 0x64, 0xff,   0xa8, 0x2e, 0x00, 0xff,   0x87, 0x4b, 0x00, 0xff, 
    0x66, 0x6b, 0x00, 0xff,   0x21, 0x83, 0x00, 0xff,   0x00, 0x8a, 0x00, 0xff,   0x00, 0x81, 0x44, 0xff, 
    0x00, 0x76, 0x91, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff, 
    0xff, 0xff, 0xff, 0xff,   0x63, 0xaf, 0xff, 0xff,   0x82, 0x96, 0xff, 0xff,   0xc0, 0x7d, 0xfe, 0xff, 
    0xe9, 0x77, 0xff, 0xff,   0xf5, 0x72, 0xcd, 0xff,   0xf4, 0x88, 0x6b, 0xff,   0xdd, 0xa0, 0x29, 0xff, 
    0xbd, 0xbd, 0x0a, 0xff,   0x89, 0xd2, 0x0e, 0xff,   0x5c, 0xde, 0x3e, 0xff,   0x4b, 0xd8, 0x86, 0xff, 
    0x4d, 0xcf, 0xd2, 0xff,   0x50, 0x50, 0x50, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff, 
    0xff, 0xff, 0xff, 0xff,   0xbe, 0xe1, 0xff, 0xff,   0xd4, 0xd4, 0xff, 0xff,   0xe3, 0xca, 0xff, 0xff, 
    0xf0, 0xc9, 0xff, 0xff,   0xff, 0xc6, 0xe3, 0xff,   0xff, 0xce, 0xc9, 0xff,   0xf4, 0xdc, 0xaf, 0xff, 
    0xeb, 0xe5, 0xa1, 0xff,   0xd2, 0xef, 0xa2, 0xff,   0xbe, 0xf4, 0xb5, 0xff,   0xb8, 0xf1, 0xd0, 0xff, 
    0xb8, 0xed, 0xf1, 0xff,   0xbd, 0xbd, 0xbd, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff
};

// TMS9919 palette.
constexpr std::array<uint8_t, 64> kMsxDefaultPalette { 
    0x00, 0x00, 0x00, 0x00, // Index 0: Transparent Black
    0x01, 0x01, 0x01, 0xFF, // Index 1: Black
    0x24, 0xDB, 0x24, 0xFF, // Index 2: Medium Green
    0x6D, 0xFF, 0x6D, 0xFF, // Index 3: Light Green
    0x24, 0x24, 0xFF, 0xFF, // Index 4: Dark Blue
    0x49, 0x6D, 0xFF, 0xFF, // Index 5: Light Blue
    0xB6, 0x24, 0x24, 0xFF, // Index 6: Dark Red
    0x49, 0xDB, 0xFF, 0xFF, // Index 7: Cyan
    0xFF, 0x24, 0x24, 0xFF, // Index 8: Medium Red
    0xFF, 0x6D, 0x6D, 0xFF, // Index 9: Light Red
    0xDB, 0xDB, 0x24, 0xFF, // Index 10: Dark Yellow
    0xDB, 0xDB, 0x92, 0xFF, // Index 11: Light Yellow
    0x24, 0x92, 0x24, 0xFF, // Index 12: Dark Green
    0xDB, 0x49, 0xB6, 0xFF, // Index 13: Magenta
    0xB6, 0xB6, 0xB6, 0xFF, // Index 14: Gray
    0xFF, 0xFF, 0xFF, 0xFF  // Index 15: White
};

// Created by master pixel artist DawnBringer, the DB16 Palette is widely considered the gold standard for general illustration, 
// sprites, and pixel art.
constexpr std::array<uint8_t, 64> kDB16Palette {
    0x00, 0x00, 0x00, 0x00, // Index 0: Transparent Black
    0x9D, 0x9D, 0x9D, 0xFF, // Index 1: GRAY
    0xFF, 0xFF, 0xFF, 0xFF, // Index 2: WHITE
    0xBE, 0x26, 0x33, 0xFF, // Index 3: RED
    0xE0, 0x6F, 0x8B, 0xFF, // Index 4: MEAT
    0x49, 0x3C, 0x2B, 0xFF, // Index 5: DARKBROWN
    0xA4, 0x64, 0x22, 0xFF, // Index 6: BROWN
    0xEB, 0x89, 0x31, 0xFF, // Index 7: ORANGE
    0xF7, 0xE2, 0x6B, 0xFF, // Index 8: YELLOW
    0x2F, 0x48, 0x4E, 0xFF, // Index 9: DARKGREEN
    0x44, 0x89, 0x1A, 0xFF, // Index 10: GREEN
    0xA3, 0xCE, 0x27, 0xFF, // Index 11: SLIMEGREEN
    0x1B, 0x26, 0x32, 0xFF, // Index 12: NIGHTBLUE
    0x00, 0x57, 0x84, 0xFF, // Index 13: SEABLUE
    0x31, 0xA2, 0xF2, 0xFF, // Index 14: SKYBLUE
    0xB2, 0xDC, 0xEF, 0xFF  // Index 15: CLOUDBLUE
};


}  // namespace Palettes

}  // namespace RetroCore

#endif  // _RETRO_CORE_FRAMEWORK_STATIC_DATA_H