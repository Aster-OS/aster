#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/idt/idt.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "arch/x86_64/pic/pic.h"
#include "kassert/kassert.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/spinlock/spinlock.h"
#include "mp/cpu.h"

static const uint8_t PIC_HANDLED_IRQ_COUNT = 8;
static const uint8_t ISA_IRQ_BASE = 0x30;
static const uint8_t FIRST_USABLE_VECTOR = 0x40;
static const uint8_t LAST_USABLE_VECTOR = 0xef;

static uint16_t last_allocated_vec = FIRST_USABLE_VECTOR;

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
    return ISA_IRQ_BASE + isa_irq;
}

void interrupts_init(void) {
    pic_disable();

    for (uint16_t vec = 0; vec < IDT_MAX_DESCRIPTORS; vec++) {
        int_handlers[vec] = unknown_int_handler;
    }

    for (uint8_t exc_vec = 0; exc_vec < 32; exc_vec++) {
        int_handlers[exc_vec] = exception_handler;
    }

    for (uint8_t pic_irq = 0; pic_irq < PIC_HANDLED_IRQ_COUNT; pic_irq++) {
        int_handlers[pic_irq + PIC1_IRQ_BASE] = pic_irq_handler;
        int_handlers[pic_irq + PIC2_IRQ_BASE] = pic_irq_handler;
    }

    klog_info("Interrupts initialized");
}

uint8_t interrupts_alloc_vector(void) {
    static struct spinlock_t lock;
    bool prev_int_state = cpu_set_int_state(false);
    spinlock_acquire(&lock);

    if (last_allocated_vec == LAST_USABLE_VECTOR) {
        kpanic("All usable vectors are exhausted");
    }

    uint8_t ret = last_allocated_vec;
    last_allocated_vec++;

    spinlock_release(&lock);
    cpu_set_int_state(prev_int_state);
    return ret;
}

void interrupts_set_handler(uint8_t vec, int_handler_t handler) {
    int_handlers[vec] = handler;
}

void interrupts_set_isa_irq_handler(uint8_t isa_irq, int_handler_t handler) {
    kassert(isa_irq < ISA_IRQ_COUNT);
    uint8_t isa_irq_vec = interrupts_get_isa_irq_vec(isa_irq);
    int_handlers[isa_irq_vec] = handler;
}
