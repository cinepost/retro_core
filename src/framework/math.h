#ifndef __RETRO_CORE_FRAMEWORK_MATH_H
#define __RETRO_CORE_FRAMEWORK_MATH_H

#include "framework/int16.h"

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>

namespace RetroCore {

constexpr unsigned int div_ceil(unsigned int x, unsigned int y) {
    assert(y > 0);
    if (x == 0) return 0;
    return 1 + ((x - 1) / y);
}

constexpr int div_ceil(int x, int y) {
    assert(y > 0);
    if (x > 0) return 1 + ((x - 1) / y);    
    return x / y; 
}

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_MATH_H