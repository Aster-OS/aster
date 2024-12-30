#include "arch/x86_64/apic/pic.h"
#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/idt/idt.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"

static int_handler_t int_handlers[IDT_MAX_DESCRIPTORS];

static void exception_handler(struct int_ctx_t *ctx) {
    kpanic_int_ctx(ctx, "Unhandled CPU Exception");
}

static void pic_irq_handler(struct int_ctx_t *ctx) {
    kpanic_int_ctx(ctx, "Unexpected PIC IRQ received", ctx->vector);
}

static void unknown_int_handler(struct int_ctx_t *ctx) {
    kpanic_int_ctx(ctx, "Received interrupt with no defined handler", ctx->vector);
}

void common_int_handler(struct int_ctx_t *ctx) {
    int_handlers[ctx->vector](ctx);
}

void interrupts_set_handler(uint8_t vector, int_handler_t handler) {
    int_handlers[vector] = handler;
}

// All vectors after those which the PIC IRQs are mapped at
// that is, after 0x20-0x27 and 0x28-0x2f,
// are free for the kernel to use
static const uint8_t FREE_VECTORS_START = 0x30;

static uint16_t last_allocated_vec = FREE_VECTORS_START;

void interrupts_init(void) {
    idt_init();
    pic_disable();

    // The first 32 interrupt vectors are reserved for CPU exceptions
    for (uint16_t exc_vec = 0; exc_vec < 32; exc_vec++) {
        int_handlers[exc_vec] = exception_handler;
    }

    // Map the PIC IRQs to their own handler
    for (uint16_t pic_irq = 0; pic_irq < PIC_IRQ_COUNT; pic_irq++) {
        int_handlers[pic_irq + PIC1_VECTOR_OFFSET] = pic_irq_handler;
        int_handlers[pic_irq + PIC2_VECTOR_OFFSET] = pic_irq_handler;
    }

    // All the other vectors are initially mapped to a handler
    // which will panic if called; the kernel should define an
    // interrupt handler and set it in the interrupt handlers array
    // before that interrupt can be properly handled
    for (uint16_t vec = FREE_VECTORS_START; vec < IDT_MAX_DESCRIPTORS; vec++) {
        int_handlers[vec] = unknown_int_handler;
    }

    enable_interrupts();

    klog_info("Interrupts initialized");
}

uint8_t interrupts_alloc_vector(void) {
    if (last_allocated_vec == IDT_MAX_DESCRIPTORS) {
        kpanic("All interrupt vectors exhausted");
    }

    uint8_t ret = last_allocated_vec;
    last_allocated_vec++;
    return ret;
}
