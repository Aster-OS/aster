#include "kprintf/kprintf.h"
#include "lib/bitmap/bitmap.h"

uint8_t bitmap_get_bit(struct bitmap_t *bitmap, uint64_t bitmap_bit) {
    uint64_t which_byte = bitmap_bit >> 3;
    uint16_t which_bit_in_byte = bitmap_bit & 0x7;
    return (bitmap->start[which_byte] & (1 << which_bit_in_byte)) >> which_bit_in_byte;
}

void bitmap_set_bit(struct bitmap_t *bitmap, uint64_t bitmap_bit) {
    uint64_t which_byte = bitmap_bit >> 3;
    uint16_t which_bit_in_byte = bitmap_bit & 0x7;
    bitmap->start[which_byte] |= 1 << which_bit_in_byte;
}

void bitmap_unset_bit(struct bitmap_t *bitmap, uint64_t bitmap_bit) {
    uint64_t which_byte = bitmap_bit >> 3;
    uint16_t which_bit_in_byte = bitmap_bit & 0x7;
    bitmap->start[which_byte] &= ~(1 << which_bit_in_byte);
}
