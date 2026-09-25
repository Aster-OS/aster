#include "arch/x86_64/idt/idt.h"

#include <stdint.h>

#include "arch/x86_64/gdt/gdt-sel.h"
#include "klog/klog.h"
#include "lib/compiler.h"

static uint8_t const IDT_INTERRUPT_GATE = 0x0e;
static uint8_t const IDT_USER = 0x60;
static uint8_t const IDT_PRESENT = 0x80;

struct idt_descriptor_t {
    uint16_t addr_0_15;
    uint16_t dest_cs;
    uint8_t ist;
    uint8_t attr;
    uint16_t addr_16_31;
    uint32_t addr_32_63;
    uint32_t reserved;
} ASTER_PACKED;

struct idtr_t {
    uint16_t limit;
    uint64_t base;
} ASTER_PACKED;

ASTER_ALIGNED(8) static struct idt_descriptor_t idt[IDT_MAX_DESCRIPTORS];
static struct idtr_t idtr;

extern void *isr_table[];

static void idt_set_descriptor(uint8_t vec, void *isr_addr) {
    uint64_t addr = (uint64_t) isr_addr;
    struct idt_descriptor_t *desc = &idt[vec];

    desc->addr_0_15 = addr & 0xffff;
    desc->dest_cs = GDT_SEL_KCODE;
    desc->ist = 0;
    desc->attr = IDT_INTERRUPT_GATE | IDT_PRESENT;
    desc->addr_16_31 = (addr >> 16) & 0xffff;
    desc->addr_32_63 = addr >> 32;
    desc->reserved = 0;
}

void idt_init(void) {
    idtr.base = (uint64_t) &idt;
    idtr.limit = sizeof(idt) - 1;

    for (uint16_t vec = 0; vec < IDT_MAX_DESCRIPTORS; vec++) {
        idt_set_descriptor(vec, isr_table[vec]);
    }

    klog_info("IDT initialized");
}

void idt_reload(void) {
    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
}

void idt_set_user(uint8_t vec) {
    struct idt_descriptor_t *desc = &idt[vec];
    desc->attr |= IDT_USER;
}
