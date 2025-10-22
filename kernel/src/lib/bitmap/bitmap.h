#pragma once

#include <stdint.h>

struct bitmap_t {
    uint8_t *start;
    uint64_t bit_count;
    // ^ uint32_t may not suffice; the bitmap would only keep track of 16384 GiB :)
};

uint8_t bitmap_get_bit(struct bitmap_t *bitmap, uint64_t bitmap_bit);
void bitmap_set_bit(struct bitmap_t *bitmap, uint64_t bitmap_bit);
void bitmap_unset_bit(struct bitmap_t *bitmap, uint64_t bitmap_bit);
