#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/msr.h"
#include "mp/cpu.h"
#include "mp/mp.h"

struct cpu_t *get_cpu(void) {
    return (struct cpu_t *) rdmsr(MSR_KERNEL_GS_BASE);
}

void set_cpu(struct cpu_t *cpu) {
    wrmsr(MSR_KERNEL_GS_BASE, (uint64_t) cpu);
}

bool cpu_set_int_state(bool interrupts_enabled) {
    disable_interrupts();
    bool interrupts_prev_enabled = get_cpu()->interrupts_enabled;
    if (interrupts_enabled) {
        get_cpu()->interrupts_enabled = true;
        enable_interrupts();
    }
    return interrupts_prev_enabled;
}
