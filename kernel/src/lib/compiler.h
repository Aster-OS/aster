#pragma once

#define ASTER_ALIGNED(x) __attribute__((aligned(x)))
#define ASTER_ALWAYS_INLINE __attribute__((always_inline))
#define ASTER_NORETURN __attribute__((noreturn))
#define ASTER_PACKED __attribute__((packed))
#define ASTER_SECTION(s) __attribute__((section(s)))
#define ASTER_UNUSED __attribute__((unused))
#define ASTER_USED __attribute__((used))
