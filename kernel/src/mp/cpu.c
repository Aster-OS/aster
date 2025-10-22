#include "arch/x86_64/asm.h"
#include "arch/x86_64/msr.h"
#include "kpanic/kpanic.h"
#include "mp/cpu.h"

bool cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    bool extended_feature_leaf = leaf >= 0x80000000;
    if (extended_feature_leaf) {
        if (leaf > get_cpu()->cpuid_extended_max) {
            return false;
        }
    } else {
        if (leaf > get_cpu()->cpuid_basic_max) {
            return false;
        }
    }

    __asm__ volatile("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx) : "a" (leaf), "c" (subleaf));
    return true;
}

void cpuid_init(void) {
    uint32_t eax, unused;
    cpuid_no_leaf_check(0, 0, &eax, &unused, &unused, &unused);
    get_cpu()->cpuid_basic_max = eax;
    cpuid_no_leaf_check(0x80000000, 0, &eax, &unused, &unused, &unused);
    get_cpu()->cpuid_extended_max = eax;
}

bool cpu_get_brand_str(char *str) {
    uint32_t *ptr = (uint32_t *) str;
    if (get_cpu()->cpuid_extended_max >= 0x80000004) {
        cpuid_no_leaf_check(0x80000002, 0, ptr,     ptr + 1, ptr + 2,  ptr + 3);
        cpuid_no_leaf_check(0x80000003, 0, ptr + 4, ptr + 5, ptr + 6,  ptr + 7);
        cpuid_no_leaf_check(0x80000004, 0, ptr + 8, ptr + 9, ptr + 11, ptr + 11);
        return true;
    }
    return false;
}

struct cpu_t *get_cpu(void) {
    if (interrupts_state()) {
        kpanic("get_cpu() called with interrupts on");
    }

    return (struct cpu_t *) rdmsr(MSR_KERNEL_GS_BASE);
}

void set_cpu(struct cpu_t *cpu) {
    wrmsr(MSR_KERNEL_GS_BASE, (uint64_t) cpu);
}
