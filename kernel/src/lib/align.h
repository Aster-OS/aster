#pragma once

static inline uint64_t align_down(uint64_t x, uint64_t align) {
    return x / align * align;
}

static inline uint64_t align_up(uint64_t x, uint64_t align) {
    return (x + align - 1) / align * align;
}

static inline uint64_t div_and_align_up(uint64_t x, uint64_t align) {
    return (x + align - 1) / align;
}
