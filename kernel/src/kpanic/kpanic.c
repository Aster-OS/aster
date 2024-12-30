#include <stdarg.h>

#include "arch/x86_64/asm_wrappers.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/printf/printf.h"

static char kpanic_buf[256];

static void do_stacktrace(void) {
    uint64_t rbp;
    __asm__ volatile("mov %%rbp, %0" : "=m" (rbp));
    klog_critical("Stacktrace:");
    uint64_t i = 0;
    while (rbp != 0) {
        uint64_t prev_rbp = *(uint64_t *) rbp;
        uint64_t ret_addr = *((uint64_t *) rbp + 1);

        klog_critical("%d %016llx", i, ret_addr);

        rbp = prev_rbp;
        i++;
    }
}

__attribute__((noreturn))
void kpanic(const char *reason, ...) {
    disable_interrupts();

    va_list va;
    va_start(va, reason);
    vsnprintf_(kpanic_buf, sizeof(kpanic_buf), reason, va);
    va_end(va);

    klog_critical("Kernel panic: %s", kpanic_buf);
    do_stacktrace();
    klog_critical("System halted");

    while (1) halt();
}
