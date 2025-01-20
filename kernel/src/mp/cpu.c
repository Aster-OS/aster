#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/msr.h"
#include "mp/cpu.h"
#include "mp/mp.h"

void cpuid_init(void) {
    uint32_t eax;
    __asm__ volatile("cpuid" : "=a" (eax) : "a" (0) : "memory", "%ebx", "%ecx", "%edx");
    get_cpu()->cpuid_basic_max = eax;
    __asm__ volatile("cpuid" : "=a" (eax) : "a" (0x80000000) : "memory", "%ebx", "%ecx", "%edx");
    get_cpu()->cpuid_extended_max = eax;
}

bool cpu_set_int_state(bool enabled) {
    disable_interrupts();
    bool interrupts_prev_enabled = get_cpu()->interrupts_enabled;
    get_cpu()->interrupts_enabled = enabled;
    if (enabled) {
        enable_interrupts();
    }
    return interrupts_prev_enabled;
}

struct cpu_t *get_cpu(void) {
    return (struct cpu_t *) rdmsr(MSR_KERNEL_GS_BASE);
}

void set_cpu(struct cpu_t *cpu) {
    wrmsr(MSR_KERNEL_GS_BASE, (uint64_t) cpu);
}
