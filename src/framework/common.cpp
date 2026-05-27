#include "framework/common.h"


namespace RetroCore {

// Compile-time validation
static_assert(bitsToBytesCount(0) == 0);
static_assert(bitsToBytesCount(1) == 1);
static_assert(bitsToBytesCount(8) == 1);
static_assert(bitsToBytesCount(9) == 2);

static_assert(getIndexMask<uint8_t, uint8_t>(15) == 4);

}  // namespace RetroCore
