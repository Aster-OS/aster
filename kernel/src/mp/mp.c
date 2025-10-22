#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/asm.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/idt/idt.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "limine.h"
#include "memory/kmalloc/kmalloc.h"
#include "memory/vmm/vmm.h"
#include "mp/cpu.h"
#include "mp/mp.h"
#include "sched/sched.h"

static struct cpu_t bsp;
static uint8_t cpu_halt_vector;
static struct cpu_t **cpus;
static uint64_t initialized_cpu_count = 1;
static bool x2apic_enabled;

static inline void init_cpu_data(struct cpu_t *cpu, uint64_t id, uint64_t acpi_id, uint64_t lapic_id) {
    cpu->id = id;
    cpu->acpi_id = acpi_id;
    cpu->lapic_id = lapic_id;
}

static void ap_entry(struct limine_mp_info *cpu_info) {
    struct cpu_t *cpu = (struct cpu_t *) cpu_info->extra_argument;
    set_cpu(cpu);

    // the kernel pagemap must be loaded before any code
    // that accesses the CPU struct, since that is on the kernel heap
    vmm_load_pagemap(vmm_get_kernel_pagemap());
    cpuid_init();
    gdt_reload_segments();
    gdt_reload_tss();
    idt_reload();
    lapic_init_cpu();
    lapic_timer_calibrate();
    sched_init_cpu();

    __atomic_fetch_add(&initialized_cpu_count, 1, __ATOMIC_SEQ_CST);

    klog_info("CPU %llu initialized", cpu->id);

    interrupts_set(true);
    sched_yield();
}

static void halt_cpu(struct int_ctx_t *ctx) {
    (void) ctx;
    interrupts_set(false);
    while (1) halt();
}

struct cpu_t *mp_get_bsp(void) {
    return &bsp;
}

uint64_t mp_get_cpu_count(void) {
    return initialized_cpu_count;
}

struct cpu_t **mp_get_cpus(void) {
    return cpus;
}

uint8_t mp_get_halt_vector(void) {
    return cpu_halt_vector;
}

void mp_init(struct limine_mp_response *mp) {
    klog_debug("x2APIC supported and enabled? %s", mp->flags & LIMINE_MP_X2APIC ? "yes" : "no");
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
            // BSP CPU struct was already initialized 
        } else {
            cpu = (struct cpu_t *) kmalloc(sizeof(struct cpu_t));
            init_cpu_data(cpu, i, cpu_info->processor_id, cpu_info->lapic_id);
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

    cpu_halt_vector = interrupts_alloc_vector();
    interrupts_set_handler(cpu_halt_vector, halt_cpu);

    klog_info("MP initialized %llu %s", initialized_cpu_count, initialized_cpu_count > 1 ? "CPUs" : "CPU");
}

void mp_init_early(struct limine_mp_response *mp) {
    x2apic_enabled = mp->flags & LIMINE_MP_X2APIC;

    for (uint64_t i = 0; i < mp->cpu_count; i++) {
        struct limine_mp_info *cpu_info = mp->cpus[i];
        bool is_bsp = cpu_info->lapic_id == mp->bsp_lapic_id;

        if (is_bsp) {
            init_cpu_data(&bsp, i, cpu_info->processor_id, cpu_info->lapic_id);
            set_cpu(&bsp);
            return;
        }
    }

    kpanic("Could not find BSP");
}

bool mp_x2apic_enabled(void) {
    return x2apic_enabled;
}
