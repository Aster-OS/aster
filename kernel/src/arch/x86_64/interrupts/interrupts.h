#pragma once

#include <stdint.h>

#define PIC1_IRQ_BASE 0x20
#define PIC2_IRQ_BASE 0x28
#define ISA_IRQ_COUNT 16

struct __attribute__((packed)) int_ctx_t {
    uint64_t cr4, cr3, cr2, cr0;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error_code, rip, cs, rflags, rsp, ss;
};

typedef void (*int_handler_t)(struct int_ctx_t *frame);

uint8_t interrupts_get_isa_irq_vec(uint8_t isa_irq);
void interrupts_init(void);
void interrupts_set_handler(uint8_t vec, int_handler_t handler);
void interrupts_set_isa_irq_handler(uint8_t isa_irq, int_handler_t handler);
