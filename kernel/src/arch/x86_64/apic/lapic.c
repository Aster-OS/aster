#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/asm_wrappers.h"
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

enum lapic_lvt_deliv_mode {
    LVT_DELIV_MODE_FIXED  = 0x0,
    LVT_DELIV_MODE_SMI    = 0x200,
    LVT_DELIV_MODE_NMI    = 0x400,
    LVT_DELIV_MODE_INIT   = 0x500,
    LVT_DELIV_MODE_EXTINT = 0x700
};

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

static uint32_t lapic_rd(uint16_t reg) {
    return *(volatile uint32_t *) (get_cpu()->lapic_addr + reg + vmm_get_hhdm_offset());
}

static void lapic_wr(uint16_t reg, uint32_t val) {
    *(volatile uint32_t *) (get_cpu()->lapic_addr + reg + vmm_get_hhdm_offset()) = val;
}

static void lapic_spurious_handler(struct int_ctx_t *ctx) {
    (void) ctx;
    klog_debug("LAPIC spurious interrupt");
}

// IDT vectors >= 0xf0 are reserved, so this vector can be used safely
static const uint8_t LAPIC_SPURIOUS_VEC = 0xf0;

static inline uint32_t ns_to_lapic_ticks(uint64_t ns) {
    return ns * get_cpu()->lapic_calibration_ticks / get_cpu()->lapic_calibration_sleep_ns;
}

void lapic_init(void) {
    uint32_t edx, unused;
    cpuid(0x1, 0x0, &unused, &unused, &unused, &edx);
    if ((edx & (1 << 9)) == 0) {
        kpanic("LAPIC not present");
    }

    get_cpu()->lapic_addr = rdmsr(MSR_IA32_APIC_BASE) & 0xffffff000;
    vmm_map_hhdm(get_cpu()->lapic_addr);

    interrupts_set_handler(LAPIC_SPURIOUS_VEC, lapic_spurious_handler);
    lapic_timer_stop();
    lapic_wr(REG_SPURIOUS, (1 << 8) | LAPIC_SPURIOUS_VEC);
    lapic_wr(REG_TIMER_DIV, 0x3); // divisor 16

    klog_info("CPU #%llu LAPIC initialized", get_cpu()->id);
}

void lapic_ipi(uint8_t vec, uint8_t dest_lapic_id) {
    lapic_wr(REG_ICR_HIGH, (uint32_t) dest_lapic_id << 24);
    lapic_wr(REG_ICR_LOW, LVT_DELIV_MODE_FIXED | vec);
}

void lapic_ipi_all(uint8_t vec) {
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
}

void lapic_ipi_all_no_self(uint8_t vec) {
    struct cpu_t *this_cpu = get_cpu();
    struct cpu_t **cpus = mp_get_cpus();
    uint64_t cpu_count = mp_get_cpu_count();
    for (uint64_t i = 0; i < cpu_count; i++) {
        struct cpu_t *cpu = cpus[i];
        if (cpu != this_cpu) {
            lapic_ipi(vec, cpu->lapic_id);
        }
    }
}

void lapic_ipi_self(uint8_t vec) {
    lapic_wr(REG_ICR_LOW, ICR_SHORTHAND_SELF | vec);
}

void lapic_send_eoi(void) {
    lapic_wr(REG_EOI, 0);
}

void lapic_timer_calibrate(uint64_t calibration_sleep_ns) {
    uint32_t calibration_start_ticks = UINT32_MAX;
    lapic_wr(REG_TIMER_INIT_COUNT, calibration_start_ticks);
    timer_sleep_ns(calibration_sleep_ns);
    uint32_t calibration_end_ticks = lapic_rd(REG_TIMER_CURR_COUNT);

    lapic_timer_stop();

    uint32_t calibration_ticks = calibration_start_ticks - calibration_end_ticks;

    get_cpu()->lapic_calibration_sleep_ns = calibration_sleep_ns;
    get_cpu()->lapic_calibration_ticks = calibration_ticks;

    klog_info("CPU #%llu LAPIC timer calibrated: %llu ticks in %llu ns", get_cpu()->id, calibration_ticks, calibration_sleep_ns);
}

void lapic_timer_one_shot(uint64_t ns, uint8_t vec) {
    uint32_t ticks = ns_to_lapic_ticks(ns);
    lapic_wr(REG_TIMER_INIT_COUNT, ticks);
    lapic_wr(REG_LVT_TIMER, LVT_TIMER_ONE_SHOT | vec);
}

void lapic_timer_periodic(uint64_t ns, uint8_t vec) {
    uint32_t ticks = ns_to_lapic_ticks(ns);
    lapic_wr(REG_TIMER_INIT_COUNT, ticks);
    lapic_wr(REG_LVT_TIMER, LVT_TIMER_PERIODIC | vec);
}

void lapic_timer_stop(void) {
    lapic_wr(REG_TIMER_INIT_COUNT, 0);
}
