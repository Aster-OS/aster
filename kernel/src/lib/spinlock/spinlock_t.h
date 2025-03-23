#pragma once

// this header is present to avoid some circular dependencies

#include <stdint.h>

#define SPINLOCK_INIT (struct spinlock_t) {0}

struct spinlock_t {
    uint8_t lock;
};
