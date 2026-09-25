#include "lib/stacktrace/stacktrace.h"

#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/interrupts/interrupts.h"
#include "klog/klog.h"
#include "lib/elf/symbols.h"

void stacktrace(struct int_ctx_t *ctx) {
    size_t depth = 0;

    uint64_t *bp, ret_addr;
    if (ctx == NULL) {
        __asm__ volatile("mov %%rbp, %0" : "=m"(bp));
        ret_addr = *(bp + 1);
        bp = (uint64_t *) *bp;
    } else {
        bp = (uint64_t *) ctx->rbp;
        ret_addr = ctx->rip;
    }

    klog_fatal("Stacktrace:");
    klog_fatal("  %zu <%s> at %016llx", depth++,
               symbols_get_func_name((void *) ret_addr), ret_addr, bp);

    while (bp && depth < 32) {
        ret_addr = *(uint64_t *) (bp + 1);

        klog_fatal("  %zu <%s> at %016llx", depth++,
                   symbols_get_func_name((void *) ret_addr), ret_addr, bp);

        bp = (uint64_t *) *bp;
    }
}
