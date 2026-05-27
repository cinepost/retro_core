#ifndef __RETRO_CORE_FRAMEWORK_COMMON_H
#define __RETRO_CORE_FRAMEWORK_COMMON_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>


namespace RetroCore {

#if defined(__clang__)
    #define UNROLL_64 _Pragma("clang loop unroll(full)")
#elif defined(__GNUC__)
    #define UNROLL_64 _Pragma("GCC unroll 64")
#else
    #define UNROLL_64 // Fallback for MSVC (relies on template or /O2)
#endif

[[nodiscard]] constexpr uint32_t bitsToBytesCount(uint32_t bits) noexcept {
    return (bits + 7) / 8;
}

template <auto A, auto B>
[[nodiscard]] constexpr auto divideExact() noexcept {
    static_assert(B != 0, "Division by zero!");
    static_assert(A % B == 0, "Division is not exact (remainder is not zero)!");
    return A / B;
}

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_COMMON_H