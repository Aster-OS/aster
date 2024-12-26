#include <stdarg.h>

#include "arch/x86_64/asm_wrappers.h"
#include "kpanic/kpanic.h"
#include "kprintf/kprintf.h"

__attribute__((noreturn))
void kpanic(const char* format, ...) {
    kprintf("KERNEL PANIC: ");

    va_list va;
    va_start(va, format);
    kvprintf(format, va);
    va_end(va);

    disable_interrupts();
    while (1) halt();
}
