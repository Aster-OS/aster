#pragma once

#include <stdint.h>

static inline void cpuid_no_leaf_check(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) : "a" (leaf), "c" (subleaf));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a" (val) : "Nd" (port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a" (val), "Nd" (port));
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

static inline void disable_interrupts(void) {
    __asm__ volatile("cli" : : : "memory");
}

static inline void enable_interrupts(void) {
    __asm__ volatile("sti" : : : "memory");
}

static inline void halt(void) {
    __asm__ volatile("hlt");
}

static inline void pause(void) {
    __asm__ volatile("pause");
}
