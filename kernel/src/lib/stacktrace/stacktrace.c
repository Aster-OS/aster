#include <stddef.h>

#include "klog/klog.h"
#include "lib/stacktrace/stacktrace.h"
#include "lib/elf/symbols.h"

void stacktrace(uint64_t bp) {
    uint64_t rbp;
    if (bp == 0) {
        __asm__ volatile("mov %%rbp, %0" : "=m" (rbp));
    } else {
        rbp = bp;
    }

    klog_fatal("Stacktrace:");

    size_t depth = 0;
    while (rbp) {
        // RBP points to the next RBP
        uint64_t *next_rbp = (uint64_t *) rbp;
        // the return address sits below the saved RBP
        // i.e. at the next higher address
        uint64_t ret_addr = *(next_rbp + 1);

        klog_fatal(" %zu <%s> at %016llx", depth, symbols_get_func_name((void *) ret_addr), ret_addr, rbp);

        rbp = *next_rbp;
        depth++;
    }
}
