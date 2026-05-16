#ifndef __RETRO_CORE_FRAMEWORK_COMMON_H
#define __RETRO_CORE_FRAMEWORK_COMMON_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>


namespace RetroCore {

[[nodiscard]] constexpr uint32_t bitsToBytesCount(uint32_t bits) noexcept {
    return (bits + 7) / 8;
}

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_COMMON_H