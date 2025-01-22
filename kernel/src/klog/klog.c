#include <stddef.h>

#include "lib/printf/printf.h"
#include "lib/spinlock/spinlock.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "mp/cpu.h"

#define LOG_FATAL_CLR "\033[31m"
#define LOG_ERROR_CLR "\033[91m"
#define LOG_WARN_CLR  "\033[93m"
#define LOG_INFO_CLR  "\033[92m"
#define LOG_DEBUG_CLR "\033[36m"
#define LOG_RESET_CLR "\033[37m"

#define TTY_MAX_COUNT 3

static struct tty_t *ttys[TTY_MAX_COUNT];
static uint8_t tty_count;
static enum klog_lvl curr_lvl;

void _putchar(char c) {
    for (uint8_t i = 0; i < tty_count; i++) {
        struct tty_t *tty = ttys[i];
        if (curr_lvl <= tty->lvl) {
            tty->putchar(c);
        }
    }
}

static inline const char *get_prefix(enum klog_lvl lvl) {
    switch (lvl) {
        case KLOG_LVL_FATAL:
            return LOG_RESET_CLR "[" LOG_FATAL_CLR "FATAL" LOG_RESET_CLR "] ";
        case KLOG_LVL_ERROR:
            return LOG_RESET_CLR "[" LOG_ERROR_CLR "ERROR" LOG_RESET_CLR "] ";
        case KLOG_LVL_WARN:
            return LOG_RESET_CLR "[" LOG_WARN_CLR "WARN" LOG_RESET_CLR "] ";
        case KLOG_LVL_INFO:
            return LOG_RESET_CLR "[" LOG_INFO_CLR "INFO" LOG_RESET_CLR "] ";
        case KLOG_LVL_DEBUG:
            return LOG_RESET_CLR "[" LOG_DEBUG_CLR "DEBUG" LOG_RESET_CLR "] ";
        default:
            kpanic("Invalid log level");
    }
}

void klog(enum klog_lvl lvl, ...) {
    static struct spinlock_t klog_lock;

    va_list va;
    va_start(va, lvl);
    const char *fmt = va_arg(va, const char *);

    bool prev_int_state = cpu_set_int_state(false);
    spinlock_acquire(&klog_lock);

    curr_lvl = lvl;

    printf_(get_prefix(lvl));
    vprintf_(fmt, va);
    _putchar('\n');

    for (size_t i = 0; i < tty_count; i++) {
        struct tty_t *tty = ttys[i];
        if (tty->do_flush) tty->flush();
    }

    spinlock_release(&klog_lock);
    cpu_set_int_state(prev_int_state);

    va_end(va);
}

void klog_register_tty(struct tty_t *tty) {
    if (tty_count == TTY_MAX_COUNT) {
        klog_warn("Exceeded max TTY count");
        return;
    }

    ttys[tty_count] = tty;
    tty_count++;
}
