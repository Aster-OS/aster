#include <stdarg.h>

#include "arch/x86_64/asm_wrappers.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/printf/printf.h"
#include "mp/mp.h"

static char kpanic_buf[256];

static void print_int_ctx(struct int_ctx_t *ctx) {
    klog_fatal("vector 0x%02x ec %x", ctx->vector, ctx->error_code);
    klog_fatal("rax %016llx    rbx %016llx", ctx->rax, ctx->rbx);
    klog_fatal("rcx %016llx    rdx %016llx", ctx->rcx, ctx->rdx);
    klog_fatal("rdi %016llx    rsi %016llx", ctx->rdi, ctx->rsi);
    klog_fatal("rbp %016llx    rsp %016llx", ctx->rbp, ctx->rsp);
    klog_fatal("r8  %016llx    r9  %016llx", ctx->r8, ctx->r9);
    klog_fatal("r10 %016llx    r11 %016llx", ctx->r10, ctx->r11);
    klog_fatal("r12 %016llx    r13 %016llx", ctx->r12, ctx->r13);
    klog_fatal("r14 %016llx    r15 %016llx", ctx->r14, ctx->r15);
    klog_fatal("cr0 %016llx    cr2 %016llx", ctx->cr0, ctx->cr2);
    klog_fatal("cr3 %016llx    cr4 %016llx", ctx->cr3, ctx->cr4);
    klog_fatal("rip %016llx rflags %016llx", ctx->rip, ctx->rflags);
    klog_fatal("cs %04x ss %04x", ctx->cs, ctx->ss);
}

static void do_stacktrace(void) {
    uint64_t rbp;
    __asm__ volatile("mov %%rbp, %0" : "=m" (rbp));
    klog_fatal("Stacktrace:");
    uint64_t i = 0;
    while (rbp != 0) {
        uint64_t prev_rbp = *(uint64_t *) rbp;
        uint64_t ret_addr = *((uint64_t *) rbp + 1);

        klog_fatal("%llu %016llx", i, ret_addr);

        rbp = prev_rbp;
        i++;
    }
}

__attribute__((noreturn))
static inline void kvpanic(struct int_ctx_t *ctx, const char *reason, va_list va) {
    vsnprintf_(kpanic_buf, sizeof(kpanic_buf), reason, va);
    klog_fatal("KERNEL PANIC on CPU #%llu >>> %s", get_cpu()->id, kpanic_buf);
    do_stacktrace();
    if (ctx != NULL) print_int_ctx(ctx);
    mp_halt_all_cpus();
    while (1) halt();
}

__attribute__((noreturn))
void kpanic(const char *reason, ...) {
    cpu_set_int_state(false);

    va_list va;
    va_start(va, reason);
    kvpanic(NULL, reason, va);
    va_end(va);
}

__attribute__((noreturn))
void kpanic_int_ctx(struct int_ctx_t *ctx, const char *reason, ...) {
    cpu_set_int_state(false);

    va_list va;
    va_start(va, reason);
    kvpanic(ctx, reason, va);
    va_end(va);
}
