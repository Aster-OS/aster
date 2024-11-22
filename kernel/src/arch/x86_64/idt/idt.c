#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/gdt/gdt_selectors.h"
#include "arch/x86_64/idt/idt.h"
#include "kprintf/kprintf.h"

#define IDT_MAX_DESCRIPTORS 256

#define IDT_DESCRIPTOR_ATTR_INTERRUPT_GATE 0xe
#define IDT_DESCRIPTOR_ATTR_PRESENT 0x80
#define IDT_DESCRIPTOR_ATTR (IDT_DESCRIPTOR_ATTR_INTERRUPT_GATE | IDT_DESCRIPTOR_ATTR_PRESENT)

struct __attribute__((packed)) idt_descriptor_t {
    uint16_t addr_0_15;
    uint16_t dest_cs;
    uint8_t ist;
    uint8_t attr;
    uint16_t addr_16_31;
    uint32_t addr_32_63;
    uint32_t reserved;
};

struct __attribute__((packed)) idtr_t {
    uint16_t limit;
    uint64_t base;
};

static __attribute__((aligned(8))) struct idt_descriptor_t idt[IDT_MAX_DESCRIPTORS];

extern void *isr_table[];

static void idt_set_descriptor(uint8_t vector, uint64_t addr, uint16_t ist) {
    struct idt_descriptor_t *idt_descriptor = &idt[vector];

    idt_descriptor->addr_0_15 = addr & 0xffff;
    idt_descriptor->dest_cs = GDT_SELECTOR_KERNEL_CODE;
    idt_descriptor->ist = ist;
    idt_descriptor->attr = IDT_DESCRIPTOR_ATTR;
    idt_descriptor->addr_16_31 = (addr >> 16) & 0xffff;
    idt_descriptor->addr_32_63 = addr >> 32;
    idt_descriptor->reserved = 0;
}

void idt_init(void) {
    struct idtr_t idtr = {
        .base = (uint64_t) &idt,
        .limit = sizeof(idt) - 1
    };

    for (size_t vector = 0; vector < IDT_MAX_DESCRIPTORS; vector++) {
        idt_set_descriptor(vector, (uint64_t) isr_table[vector], 0);
    }
    
    __asm__ volatile("lidt %0; sti;" : : "m" (idtr) : "memory");

    kprintf("IDT initialized\n");    
}
