#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/idt/idt.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "memory/kheap/kheap.h"
#include "memory/vmm/vmm.h"
#include "mp/cpu.h"
#include "mp/mp.h"

static struct cpu_t bsp_cpu;
static struct cpu_t **cpus;
static uint64_t initialized_ap_count;

struct cpu_t *mp_get_bsp(void) {
    return &bsp_cpu;
}

void mp_init_bsp(struct limine_mp_response *mp) {
    for (uint64_t i = 0; i < mp->cpu_count; i++) {
        struct limine_mp_info *limine_cpu_info = mp->cpus[i];
        bool is_bsp = limine_cpu_info->lapic_id == mp->bsp_lapic_id;

        if (is_bsp) {
            bsp_cpu.id = i;
            bsp_cpu.lapic_id = limine_cpu_info->lapic_id;
            bsp_cpu.acpi_id = limine_cpu_info->processor_id;
            bsp_cpu.interrupts_enabled = false;
        }

        set_cpu(&bsp_cpu);
        return;
    }

    kpanic("Could not find BSP");
}

static void ap_init(struct limine_mp_info *cpu_info) {
    struct cpu_t *cpu = (struct cpu_t *) cpu_info->extra_argument;

    set_cpu(cpu);

    vmm_load_pagemap(vmm_get_kernel_pagemap());
    gdt_reload_segments();
    gdt_reload_tss();
    idt_reload();
    lapic_init();
    lapic_timer_calibrate(1000000);
    cpu_set_int_state(true);

    klog_info("CPU %u initialized", cpu->id, cpu->lapic_id, cpu->acpi_id);

    __atomic_fetch_add(&initialized_ap_count, 1, __ATOMIC_SEQ_CST);

    disable_interrupts();
    while (1) halt();
}

void mp_init_aps(struct limine_mp_response *mp) {
    klog_debug("x2APIC enabled: %s", mp->flags & LIMINE_MP_X2APIC ? "yes" : "no");
    klog_debug("BSP LAPIC ID: %d", mp->bsp_lapic_id);

    cpus = (struct cpu_t **) kheap_alloc(mp->cpu_count * sizeof(struct cpu_t *));

    if (mp->cpu_count == 1) {
        klog_info("No APs to initialize");
        return;
    }

    for (uint64_t i = 0; i < mp->cpu_count; i++) {
        struct limine_mp_info *limine_cpu_info = mp->cpus[i];
        bool is_bsp = limine_cpu_info->lapic_id == mp->bsp_lapic_id;

        struct cpu_t *cpu;
        if (is_bsp) {
            cpu = &bsp_cpu;
        } else {
            cpu = (struct cpu_t *) kheap_alloc(sizeof(struct cpu_t));
        }

        cpu->id = i;
        cpu->lapic_id = limine_cpu_info->lapic_id;
        cpu->acpi_id = limine_cpu_info->processor_id;
        cpu->interrupts_enabled = false;
        cpus[i] = cpu;

        if (is_bsp) {
            continue;
        }

        limine_cpu_info->extra_argument = (uint64_t) cpu;
        __atomic_store_n(&limine_cpu_info->goto_address, ap_init, __ATOMIC_SEQ_CST);
    }

    while (__atomic_load_n(&initialized_ap_count, __ATOMIC_SEQ_CST) != (mp->cpu_count - 1)) {
        pause();
    }

    klog_info("MP initialized %llu %s, system has %llu CPUs",
        initialized_ap_count, initialized_ap_count > 1 ? "APs" : "AP", mp->cpu_count);
}
