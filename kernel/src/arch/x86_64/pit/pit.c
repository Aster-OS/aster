#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/apic/ioapic.h"
#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "arch/x86_64/pit/pit.h"
#include "kassert/kassert.h"
#include "klog/klog.h"

static const uint8_t PIT_CH0_DATA_PORT = 0x40;
static const uint8_t PIT_COMMAND_PORT = 0x43;

static const uint64_t PIT_INTERNAL_FREQ = 1193182;
static const uint64_t PIT_DESIRED_FREQ  = 1000;

static const uint8_t PIT_ISA_IRQ = 0;

static uint64_t ticks;

static inline uint64_t ns_to_ticks(uint64_t ns) {
    return ns * PIT_DESIRED_FREQ / 1000000;
}

static inline uint64_t ticks_to_ns(uint64_t ticks) {
    return 1000000 * ticks / PIT_DESIRED_FREQ;
}

static void pit_int_handler(struct int_ctx_t *ctx) {
    (void) ctx;
    ticks++;
    lapic_send_eoi();
}

uint64_t pit_get_ns(void) {
    return ticks_to_ns(ticks);
}

void pit_init(void) {
    outb(PIT_COMMAND_PORT, 0x34);

    uint16_t freq_divisor = PIT_INTERNAL_FREQ / PIT_DESIRED_FREQ;

    kassert(freq_divisor != 1);

    outb(PIT_CH0_DATA_PORT, freq_divisor & 0xff);
    outb(PIT_CH0_DATA_PORT, freq_divisor >> 8);

    interrupts_set_isa_irq_handler(PIT_ISA_IRQ, pit_int_handler);
    ioapic_unmask_isa_irq(PIT_ISA_IRQ);

    klog_debug("PIT initialized");
}

void pit_sleep_ns(uint64_t ns) {
    uint64_t ticks_to_sleep = ns_to_ticks(ns);
    uint64_t ticks_start = ticks;
    uint64_t ticks_end = ticks_start + ticks_to_sleep;
    while (ticks < ticks_end) {
        pause();
    }
}
