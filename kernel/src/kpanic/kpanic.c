#include <stdarg.h>

#include "kpanic/kpanic.h"
#include "kprintf/kprintf.h"

__attribute__((noreturn))
void kpanic(const char* reason) {
    kprintf("KERNEL PANIC! REASON: %s\n", reason);
    __asm__ volatile("cli; hlt");
    while (1);
}
