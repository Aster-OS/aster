#include "arch/x86_64/idt/idt.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "arch/x86_64/pic/pic.h"
#include "kassert/kassert.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/spinlock/spinlock.h"

// 0x00 - 0x1f | CPU exceptions
// 0x20 - 0x27 | PIC 1 IRQs [ignored]
// 0x28 - 0x2f | PIC 2 IRQs [ignored]
// 0x30 - 0x3f | ISA IRQs
// 0x40 - 0xef | Usable
// 0xf0 - 0xff | Reserved for kernel use
// 0xf0 - LAPIC spurious interrupt

static const uint8_t IRQ_COUNT_PER_PIC = 8;
static const uint8_t ISA_IRQ_OFFSET = 0x30;

static const uint8_t USABLE_VECTORS_START = 0x40;
static const uint8_t USABLE_VECTORS_END   = 0xef;

static uint16_t curr_free_vector = USABLE_VECTORS_START;

// interrupt handlers shared across all CPUs
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

uint8_t interrupts_get_isa_irq_vec(uint8_t isa_irq) {
    return ISA_IRQ_OFFSET + isa_irq;
}

void interrupts_init(void) {
    pic_disable();

    for (uint16_t vec = 0; vec < IDT_MAX_DESCRIPTORS; vec++) {
        int_handlers[vec] = unknown_int_handler;
    }

    for (uint8_t exc_vec = 0; exc_vec < 32; exc_vec++) {
        int_handlers[exc_vec] = exception_handler;
    }

    for (uint8_t pic_irq = 0; pic_irq < IRQ_COUNT_PER_PIC; pic_irq++) {
        int_handlers[pic_irq + PIC1_IRQ_OFFSET] = pic_irq_handler;
        int_handlers[pic_irq + PIC2_IRQ_OFFSET] = pic_irq_handler;
    }

    klog_info("Interrupts initialized");
}

uint8_t interrupts_alloc_vector(void) {
    static struct spinlock_t lock = SPINLOCK_STATIC_INIT;
    spin_lock_irqsave(&lock);

    if (curr_free_vector == USABLE_VECTORS_END) {
        kpanic("All usable vectors are exhausted");
    }

    uint8_t ret = curr_free_vector;
    curr_free_vector++;

    spin_unlock_irqrestore(&lock);

    return ret;
}

void interrupts_set_handler(uint8_t vec, int_handler_t handler) {
    int_handlers[vec] = handler;
}

void interrupts_set_isa_irq_handler(uint8_t isa_irq, int_handler_t handler) {
    kassert(isa_irq < ISA_IRQ_MAX);
    uint8_t isa_irq_vec = interrupts_get_isa_irq_vec(isa_irq);
    int_handlers[isa_irq_vec] = handler;
}
