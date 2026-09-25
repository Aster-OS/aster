#include "syscall.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/interrupts/interrupts.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "mp/cpu.h"
#include "sched/sched.h"

#define SYSCALL_MAX 0x100

typedef uint64_t (*syscall_handler_t)(struct int_ctx_t *ctx);

syscall_handler_t syscall_table[SYSCALL_MAX + 1];

#define syscall_arg0(ctx) (ctx->rdi)
#define syscall_arg1(ctx) (ctx->rsi)
#define syscall_arg2(ctx) (ctx->rdx)
#define syscall_arg3(ctx) (ctx->r10)
#define syscall_arg4(ctx) (ctx->r8)
#define syscall_arg5(ctx) (ctx->r9)

static void syscall_register(size_t num, syscall_handler_t handler) {
    if (num > SYSCALL_MAX) {
        kpanic("Could not register syscall");
    }

    syscall_table[num] = handler;
}

uint64_t syscall_write(struct int_ctx_t *ctx) {
    unsigned int fd = syscall_arg0(ctx);
    (void) fd;
    char const *buf = (char const *) syscall_arg1(ctx);
    size_t count = syscall_arg2(ctx);

    if (fd != 1) {
        return 1;
    }

    // TODO: Check address in lower half, no overflow

    klog_info("syscall_write: %.*s", count, buf);

    return 0;
}

uint64_t syscall_exit(struct int_ctx_t *ctx) {
    int error_code = syscall_arg0(ctx);
    klog_info("syscall_exit: tid=%u, cpu=%u, code=%d",
              get_cpu()->curr_thread->tid, get_cpu()->id, error_code);
    sched_exit();
    return 0;
}

void syscall_common(struct int_ctx_t *ctx) {
    size_t num = ctx->rax;
    if (num > SYSCALL_MAX || syscall_table[num] == NULL) {
        klog_warn("Syscall not implemented");
        return;
    }

    syscall_handler_t handler = (syscall_handler_t) syscall_table[num];
    uint64_t ret = handler(ctx);
    ctx->rax = ret;
}

void syscall_init(void) {
    syscall_register(1, syscall_write);
    syscall_register(60, syscall_exit);

    interrupts_set_handler(0xf1, syscall_common, true);
}
