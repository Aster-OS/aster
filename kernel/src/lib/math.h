#include <stdint.h>

#include "lib/compiler.h"

static inline ASTER_ALWAYS_INLINE uint64_t min(uint64_t x, uint64_t y) {
    return x < y ? x : y;
}

static inline ASTER_ALWAYS_INLINE uint64_t max(uint64_t x, uint64_t y) {
    return x > y ? x : y;
}
