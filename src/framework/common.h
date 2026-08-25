#ifndef __RETRO_CORE_FRAMEWORK_COMMON_H
#define __RETRO_CORE_FRAMEWORK_COMMON_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>


#if defined(_MSC_VER)
    #define FORCE_INLINE __forceinline  // MSVC (Windows)
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCE_INLINE inline __attribute__((always_inline)) // GCC/Clang (Linux/Mac)
#else
    #define FORCE_INLINE inline
#endif


#if defined(__clang__)
    #define UNROLL_64 _Pragma("clang loop unroll(full)")
#elif defined(__GNUC__)
    #define UNROLL_64 _Pragma("GCC unroll 64")
#else
    #define UNROLL_64 // Fallback for MSVC (relies on template or /O2)
#endif


namespace RetroCore {

[[nodiscard]] constexpr bool isPowerOfTwo(size_t n) noexcept {
    return n > 0 && (n & (n - 1)) == 0;
}

[[nodiscard]] constexpr uint8_t bitsToBytesCount(uint8_t bits) noexcept {
    return (bits + 7) / 8;
}

template <auto A, auto B>
[[nodiscard]] constexpr auto divideExact() noexcept {
    static_assert(B != 0, "Division by zero!");
    static_assert(A % B == 0, "Division is not exact (remainder is not zero)!");
    return A / B;
}

template <typename M, typename T>
[[nodiscard]] constexpr M getIndexMask(T max_index) noexcept {
    M mask = 0;
    uint32_t value = (uint32_t)(max_index);
    while (value) {
        mask += (value & 1);
        value >>= 1;
    }
    return mask;
}

}  // namespace RetroCore

#endif  // __RETRO_CORE_FRAMEWORK_COMMON_H