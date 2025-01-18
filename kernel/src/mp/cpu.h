#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "arch/x86_64/gdt/gdt.h"

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
};

bool cpu_set_int_state(bool enabled);

struct cpu_t *get_cpu(void);
void set_cpu(struct cpu_t *cpu);
