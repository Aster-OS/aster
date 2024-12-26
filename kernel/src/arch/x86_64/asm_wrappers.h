#pragma once

#include <stdint.h>

static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) : "a" (leaf), "c" (subleaf));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a" (val) : "Nd" (port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" :: "a" (val), "Nd" (port));
}

static inline void invlpg(uintptr_t addr) {
    __asm__ volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

static inline uint64_t rd_cr3(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3) : : "memory");
    return cr3;
}

static inline void wr_cr3(uint64_t cr3) {
    __asm__ volatile("mov %0, %%cr3" : : "r" (cr3) : "memory");
}

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

static inline void disable_interrupts(void) {
    __asm__ volatile("cli");
}

static inline void enable_interrupts(void) {
    __asm__ volatile("sti");
}

static inline void halt(void) {
    __asm__ volatile("hlt");
}
