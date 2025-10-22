#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "arch/x86_64/asm.h"

#define SPINLOCK_INIT ((struct spinlock_t) {0, 0})
#define SPINLOCK_STATIC_INIT {0, 0}

struct spinlock_t {
    uint8_t lock;
    bool old_int_state;
};

static inline void spin_lock(struct spinlock_t *spinlock) {
    while (__atomic_test_and_set(&spinlock->lock, __ATOMIC_ACQUIRE)) {
        __asm__ volatile("pause");
    }
}

static inline void spin_unlock(struct spinlock_t *spinlock) {
    __atomic_clear(&spinlock->lock, __ATOMIC_RELEASE);
}

static inline void spin_lock_irqsave(struct spinlock_t *spinlock) {
    bool old_int_state = interrupts_set(false);
    spin_lock(spinlock);
    spinlock->old_int_state = old_int_state;
}

static inline void spin_unlock_irqrestore(struct spinlock_t *spinlock) {
    bool old_int_state = spinlock->old_int_state;
    spin_unlock(spinlock);
    interrupts_set(old_int_state);
}
