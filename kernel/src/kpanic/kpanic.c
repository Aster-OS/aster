#include <stdarg.h>

#include "kpanic/kpanic.h"
#include "kprintf/kprintf.h"

__attribute__((noreturn))
void kpanic(const char* format, ...) {
    kprintf("KERNEL PANIC: ");

    va_list va;
    va_start(va, format);
    kvprintf(format, va);
    va_end(va);

    __asm__ volatile("cli; hlt");
    while (1);
}
