#include "acpi/madt.h"
#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "arch/x86_64/msr.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "memory/vmm/vmm.h"
#include "mp/mp.h"
#include "timer/timer.h"

enum lapic_regs {
    REG_LAPIC_ID = 0x20,
    REG_LAPIC_VER = 0x30,
    REG_TPR = 0x80,
    REG_EOI = 0xb0,
    REG_SPURIOUS = 0xf0,
    REG_ICR_LOW = 0x300,
    REG_ICR_HIGH = 0x310,
    REG_LVT_TIMER = 0x320,
    REG_LVT_LINT0 = 0x350,
    REG_LVT_LINT1 = 0x360,
    REG_TIMER_INIT_COUNT = 0x380,
    REG_TIMER_CURR_COUNT = 0x390,
    REG_TIMER_DIV = 0x3e0
};

static const uint16_t X2APIC_REG_SELF_IPI = 0x83f;

enum lapic_lvt_deliv_status {
    LVT_DELIV_STATUS_SENT    = 0x0,
    LVT_DELIV_STATUS_PENDING = 0x1000
};

enum lapic_lvt_pin_polarity {
    LVT_ACTIVE_HIGH = 0x0,
    LVT_ACTIVE_LOW  = 0x2000
};

enum lapic_lvt_trigger_mode {
    LVT_TRIGGER_EDGE  = 0x0,
    LVT_TRIGGER_LEVEL = 0x8000
};

enum lapic_deliv_mode {
    LAPIC_DELIV_MODE_FIXED  = 0x0,
    LAPIC_DELIV_MODE_SMI    = 0x200,
    LAPIC_DELIV_MODE_NMI    = 0x400,
    LAPIC_DELIV_MODE_INIT   = 0x500,
    LAPIC_DELIV_MODE_EXTINT = 0x700
};

__attribute__((used))
static const uint32_t LVT_MASKED = 0x10000;

enum lapic_lvt_timer_mode {
    LVT_TIMER_ONE_SHOT     = 0x0,
    LVT_TIMER_PERIODIC     = 0x20000,
    LVT_TIMER_TSC_DEADLINE = 0x40000
};

enum lapic_icr_dest_mode {
    ICR_DEST_MODE_PHYSICAL = 0x0,
    ICR_DEST_MODE_LOGICAL  = 0x800
};

enum lapic_icr_shorthand {
    ICR_SHORTHAND_SELF        = 0x40000,
    ICR_SHORTHAND_ALL         = 0x80000,
    ICR_SHORTHAND_ALL_NO_SELF = 0xc0000
};

static const uint64_t LAPIC_CALIBRATION_NS = 100000;
static const uint8_t LAPIC_SPURIOUS_VEC = 0xf0;

static uint64_t lapic_addr;

static inline uint16_t reg_to_x2apic_msr(uint16_t reg) {
    return (reg >> 4) + 0x800;
}

static inline uint32_t lapic_read(uint16_t reg) {
    if (mp_x2apic_enabled()) {
        return rdmsr(reg_to_x2apic_msr(reg));
    } else {
        return *(volatile uint32_t *) (lapic_addr + reg + vmm_get_hhdm_offset());
    }
}

static inline void lapic_write(uint16_t reg, uint32_t val) {
    if (mp_x2apic_enabled()) {
        wrmsr(reg_to_x2apic_msr(reg), val);
    } else {
        *(volatile uint32_t *) (lapic_addr + reg + vmm_get_hhdm_offset()) = val;
    }
}

static void lapic_spurious_handler(struct int_ctx_t *ctx) {
    (void) ctx;
    klog_debug("LAPIC spurious interrupt");
}

static inline uint32_t ns_to_lapic_ticks(uint64_t ns) {
    return ns * get_cpu()->lapic_calibration_ticks / LAPIC_CALIBRATION_NS;
}

void lapic_init(void) {
    lapic_addr = rdmsr(MSR_IA32_APIC_BASE) & 0xffffff000;
    vmm_map_hhdm(lapic_addr);

    interrupts_set_handler(LAPIC_SPURIOUS_VEC, lapic_spurious_handler);
}

void lapic_init_cpu(void) {
    uint32_t edx, unused;
    if (!cpuid(0x1, 0x0, &unused, &unused, &unused, &edx) || (edx & (1 << 9)) == 0) {
        kpanic("CPU does not have a LAPIC");
    }

    lapic_timer_stop();
    lapic_write(REG_SPURIOUS, (1 << 8) | LAPIC_SPURIOUS_VEC);
    lapic_write(REG_TIMER_DIV, 0x3); // divisor 16

    struct lapic_nmi_t **lapic_nmis = madt_get_lapic_nmis();
    uint64_t lapic_nmi_count = madt_get_lapic_nmi_count();
    for (uint64_t i = 0; i < lapic_nmi_count; i++) {
        struct lapic_nmi_t *lapic_nmi = lapic_nmis[i];
        if (lapic_nmi->acpi_id == 0xff || lapic_nmi->acpi_id == get_cpu()->acpi_id) {
            uint16_t lvt_flags = 0;
            if ((lapic_nmi->flags & MADT_ACTIVE_HIGH) == MADT_ACTIVE_HIGH) {
                lvt_flags = LVT_ACTIVE_HIGH;
            } else if ((lapic_nmi->flags & MADT_ACTIVE_LOW) == MADT_ACTIVE_LOW) {
                lvt_flags = LVT_ACTIVE_LOW;
            }

            // trigger mode is always edge sensitive for NMI delivery mode

            if (lapic_nmi->lint == 0) {
                lapic_write(REG_LVT_LINT0, lvt_flags | LAPIC_DELIV_MODE_NMI | 0x4);
            } else if (lapic_nmi->lint == 1) {
                lapic_write(REG_LVT_LINT1, lvt_flags | LAPIC_DELIV_MODE_NMI | 0x4);
            }
        }
    }

    klog_info("CPU %llu LAPIC initialized", get_cpu()->id);
}

void lapic_ipi(uint8_t vec, uint32_t dest_lapic_id) {
    if (mp_x2apic_enabled()) {
        uint64_t icr = ((uint64_t) dest_lapic_id << 32) | LAPIC_DELIV_MODE_FIXED | vec;
        // ICR is 64 bits in x2APIC mode
        wrmsr(reg_to_x2apic_msr(REG_ICR_LOW), icr);
    } else {
        uint64_t icr = ((uint64_t) dest_lapic_id << 56) | LAPIC_DELIV_MODE_FIXED | vec;
        lapic_write(REG_ICR_HIGH, icr >> 32);
        lapic_write(REG_ICR_LOW, icr & 0xffff);
    }
}

void lapic_ipi_all(uint8_t vec) {
    bool old_int_state = interrupts_set(false);
    struct cpu_t *this_cpu = get_cpu();
    struct cpu_t **cpus = mp_get_cpus();
    uint64_t cpu_count = mp_get_cpu_count();
    for (uint64_t i = 0; i < cpu_count; i++) {
        struct cpu_t *cpu = cpus[i];
        if (cpu != this_cpu) {
            lapic_ipi(vec, cpu->lapic_id);
        }
    }
    lapic_ipi(vec, this_cpu->lapic_id);
    interrupts_set(old_int_state);
}

void lapic_ipi_all_no_self(uint8_t vec) {
    bool old_int_state = interrupts_set(false);
    struct cpu_t *this_cpu = get_cpu();
    struct cpu_t **cpus = mp_get_cpus();
    uint64_t cpu_count = mp_get_cpu_count();
    for (uint64_t i = 0; i < cpu_count; i++) {
        struct cpu_t *cpu = cpus[i];
        if (cpu != this_cpu) {
            lapic_ipi(vec, cpu->lapic_id);
        }
    }
    interrupts_set(old_int_state);
}

void lapic_ipi_self(uint8_t vec) {
    if (mp_x2apic_enabled()) {
        wrmsr(X2APIC_REG_SELF_IPI, vec);
    } else {
        uint64_t icr = ICR_SHORTHAND_SELF | LAPIC_DELIV_MODE_FIXED | vec;
        lapic_write(REG_ICR_HIGH, icr >> 32);
        lapic_write(REG_ICR_LOW, icr & 0xffff);
    }
}

void lapic_send_eoi(void) {
    lapic_write(REG_EOI, 0);
}

void lapic_timer_calibrate(void) {
    interrupts_set(true);
    uint32_t start_ticks = UINT32_MAX;
    lapic_write(REG_TIMER_INIT_COUNT, start_ticks);
    timer_sleep_ns(LAPIC_CALIBRATION_NS);
    uint32_t end_ticks = lapic_read(REG_TIMER_CURR_COUNT);
    lapic_timer_stop();
    interrupts_set(false);

    get_cpu()->lapic_calibration_ticks = start_ticks - end_ticks;

    klog_info("CPU %llu LAPIC timer calibrated: %llu ticks in %llu ns",
            get_cpu()->id, get_cpu()->lapic_calibration_ticks, LAPIC_CALIBRATION_NS);
}

void lapic_timer_one_shot(uint64_t ns, uint8_t vec) {
    uint32_t ticks = ns_to_lapic_ticks(ns);
    lapic_write(REG_TIMER_INIT_COUNT, ticks);
    lapic_write(REG_LVT_TIMER, LVT_TIMER_ONE_SHOT | vec);
}

void lapic_timer_periodic(uint64_t ns, uint8_t vec) {
    uint32_t ticks = ns_to_lapic_ticks(ns);
    lapic_write(REG_TIMER_INIT_COUNT, ticks);
    lapic_write(REG_LVT_TIMER, LVT_TIMER_PERIODIC | vec);
}

void lapic_timer_stop(void) {
    lapic_write(REG_TIMER_INIT_COUNT, 0);
}
