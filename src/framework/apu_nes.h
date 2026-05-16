#ifndef __RETRO_CORE_FRAMEWORK_APU_NES_H
#define __RETRO_CORE_FRAMEWORK_APU_NES_H

#include "framework/apu_base.h"

namespace RetroCore {

struct NESTables {
    // Fixed 32-step wave lookup sequence for the hardware triangle generator
    static constexpr std::array<int16_t, 32> triangle_sequence = {
        15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    };

    // Four hardware duty configurations for Pulse waves: 12.5%, 25%, 50%, 75%
    static constexpr std::array<std::array<uint8_t, 8>, 4> pulse_duties = {{
        {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
        {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
        {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
        {1, 0, 0, 1, 1, 1, 1, 1}  // 75% (Inverted 25%)
    }};
};

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_APU_NES_H