#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "arch/x86_64/gdt/gdt.h"
#include "lib/list/dlist.h"
#include "sched/thread.h"

DLIST_TYPE_ATOMIC(thread_queue_t, struct thread_t);

struct cpu_t {
    uint64_t id;
    uint64_t acpi_id;
    uint64_t lapic_id;
    uint64_t lapic_addr;
    uint64_t lapic_calibration_ns;
    uint64_t lapic_calibration_ticks;
    bool x2apic_enabled;
    struct tss_t tss;
    bool interrupts_enabled;
    uint32_t cpuid_basic_max;
    uint32_t cpuid_extended_max;
    struct thread_t *curr_thread;
    struct thread_queue_t dead_queue;
    struct thread_queue_t run_queue;
};

bool cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
void cpuid_init(void);

bool cpu_get_brand_str(char *str);
bool cpu_set_int_state(bool enabled);

struct cpu_t *get_cpu(void);
void set_cpu(struct cpu_t *cpu);
