#pragma once

#include <stddef.h>

#include "arch/x86_64/asm_wrappers.h"
#include "klog/klog.h"
#include "lib/spinlock/spinlock_t.h"

static inline void spinlock_acquire(struct spinlock_t *spinlock) {
    while (__atomic_test_and_set(&spinlock->lock, __ATOMIC_ACQUIRE)) {
        pause();
    }
}

static inline void spinlock_release(struct spinlock_t *spinlock) {
    __atomic_clear(&spinlock->lock, __ATOMIC_RELEASE);
}
