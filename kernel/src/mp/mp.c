#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/idt/idt.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "memory/kmalloc/kmalloc.h"
#include "memory/vmm/vmm.h"
#include "mp/cpu.h"
#include "mp/mp.h"
#include "sched/sched.h"

static struct cpu_t bsp;
static struct cpu_t **cpus;
static uint64_t initialized_cpu_count = 1;

static const uint64_t LAPIC_CALIBRATION_NS = 100000;

static inline void init_cpu_data(struct cpu_t *cpu,
                                uint64_t id, uint64_t acpi_id, uint64_t lapic_id,
                                uint64_t lapic_calibration_ns, bool x2apic_enabled) {
    cpu->id = id;
    cpu->acpi_id = acpi_id;
    cpu->lapic_id = lapic_id;
    cpu->lapic_calibration_ns = lapic_calibration_ns;
    cpu->x2apic_enabled = x2apic_enabled;
    cpu->interrupts_enabled = false;
}

struct cpu_t *mp_get_bsp(void) {
    return &bsp;
}

void mp_init_bsp(struct limine_mp_response *mp) {
    for (uint64_t i = 0; i < mp->cpu_count; i++) {
        struct limine_mp_info *cpu_info = mp->cpus[i];
        bool is_bsp = cpu_info->lapic_id == mp->bsp_lapic_id;

        if (is_bsp) {
            init_cpu_data(&bsp,
                            i, cpu_info->processor_id, cpu_info->lapic_id,
                            LAPIC_CALIBRATION_NS, mp->flags & LIMINE_MP_X2APIC);
            set_cpu(&bsp);
            return;
        }
    }

    kpanic("Could not find BSP");
}

uint64_t mp_get_cpu_count(void) {
    return initialized_cpu_count;
}

struct cpu_t **mp_get_cpus(void) {
    return cpus;
}

static void ap_entry(struct limine_mp_info *cpu_info) {
    struct cpu_t *cpu = (struct cpu_t *) cpu_info->extra_argument;

    set_cpu(cpu);

    // the kernel pagemap must be loaded before any code
    // that R/W from/to the CPU struct because it is allocated on the heap
    vmm_load_pagemap(vmm_get_kernel_pagemap());

    cpuid_init();
    gdt_reload_segments();
    gdt_reload_tss();
    idt_reload();
    lapic_init();
    lapic_timer_calibrate();
    cpu_set_int_state(true);
    sched_init_cpu();

    __atomic_fetch_add(&initialized_cpu_count, 1, __ATOMIC_SEQ_CST);

    klog_info("CPU #%llu initialized", cpu->id);

    sched_yield();
}

static void halt_cpu(struct int_ctx_t *ctx) {
    (void) ctx;
    cpu_set_int_state(false);
    klog_fatal("CPU #%llu halted", get_cpu()->id);
    while (1) halt();
}

__attribute__((noreturn))
void mp_halt_all_cpus(void) {
    klog_fatal("Halting all %llu CPUs...", initialized_cpu_count);
    // a call to this function may happen very early
    // when the LAPIC isn't even initialized
    if (get_cpu()->lapic_addr) {
        uint8_t halt_cpu_vector = interrupts_alloc_vector();
        interrupts_set_handler(halt_cpu_vector, halt_cpu);
        lapic_ipi_all(halt_cpu_vector);
        cpu_set_int_state(true);
    }
    while (1) halt();
}

void mp_init(struct limine_mp_response *mp) {
    klog_debug("x2APIC enabled? %s", mp->flags & LIMINE_MP_X2APIC ? "yes" : "no");
    cpus = (struct cpu_t **) kmalloc(mp->cpu_count * sizeof(struct cpu_t *));

    if (mp->cpu_count == 1) {
        klog_info("No APs to initialize");
        cpus[0] = &bsp;
        return;
    }

    for (uint64_t i = 0; i < mp->cpu_count; i++) {
        struct limine_mp_info *cpu_info = mp->cpus[i];
        bool is_bsp = cpu_info->lapic_id == mp->bsp_lapic_id;

        struct cpu_t *cpu;
        if (is_bsp) {
            cpu = &bsp;
            // bsp data was already initialized
        } else {
            cpu = (struct cpu_t *) kmalloc(sizeof(struct cpu_t));
            init_cpu_data(cpu,
                            i, cpu_info->processor_id, cpu_info->lapic_id,
                            LAPIC_CALIBRATION_NS, mp->flags & LIMINE_MP_X2APIC);
        }

        cpus[i] = cpu;

        if (is_bsp) {
            continue;
        }

        cpu_info->extra_argument = (uint64_t) cpu;
        __atomic_store_n(&cpu_info->goto_address, ap_entry, __ATOMIC_SEQ_CST);
    }

    while (__atomic_load_n(&initialized_cpu_count, __ATOMIC_SEQ_CST) != mp->cpu_count) {
        pause();
    }

    klog_info("MP initialized %llu %s", initialized_cpu_count, initialized_cpu_count > 1 ? "CPUs" : "CPU");
}
