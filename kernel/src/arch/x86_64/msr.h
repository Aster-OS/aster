#pragma once

#include <stdint.h>

#define MSR_IA32_APIC_BASE 0x1b
#define MSR_GS_BASE 0xc0000101
#define MSR_KERNEL_GS_BASE 0xc0000102

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t eax, edx;
    __asm__ volatile("rdmsr" : "=a" (eax), "=d" (edx) : "c" (msr) : "memory");
    return ((uint64_t) edx << 32) | eax;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t eax = val;
    uint32_t edx = val >> 32;
    __asm__ volatile("wrmsr" : : "a" (eax), "d" (edx), "c" (msr) : "memory");
}
