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

static void print_int_ctx(struct int_ctx_t *ctx) {
    klog_critical("vector %02x ec %x", ctx->vector, ctx->error_code);
    klog_critical("rax %016llx    rbx %016llx", ctx->rax, ctx->rbx);
    klog_critical("rcx %016llx    rdx %016llx", ctx->rcx, ctx->rdx);
    klog_critical("rdi %016llx    rsi %016llx", ctx->rdi, ctx->rsi);
    klog_critical("rbp %016llx    rsp %016llx", ctx->rbp, ctx->rsp);
    klog_critical("r8  %016llx    r9  %016llx", ctx->r8, ctx->r9);
    klog_critical("r10 %016llx    r11 %016llx", ctx->r10, ctx->r11);
    klog_critical("r12 %016llx    r13 %016llx", ctx->r12, ctx->r13);
    klog_critical("r14 %016llx    r15 %016llx", ctx->r14, ctx->r15);
    klog_critical("cr0 %016llx    cr2 %016llx", ctx->cr0, ctx->cr2);
    klog_critical("cr3 %016llx    cr4 %016llx", ctx->cr3, ctx->cr4);
    klog_critical("rip %016llx rflags %016llx", ctx->rip, ctx->rflags);
    klog_critical("cs %04x ss %04x", ctx->cs, ctx->ss);
}

__attribute__((noreturn))
void kpanic_int_ctx(struct int_ctx_t *ctx, const char *reason, ...) {
    disable_interrupts();

    va_list va;
    va_start(va, reason);
    vsnprintf_(kpanic_buf, sizeof(kpanic_buf), reason, va);
    va_end(va);

    klog_critical("Kernel panic: %s", kpanic_buf);
    do_stacktrace();
    print_int_ctx(ctx);
    klog_critical("System halted");

    while (1) halt();
}
