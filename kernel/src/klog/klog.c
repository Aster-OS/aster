#include <stddef.h>

#include "lib/flanterm/flanterm.h"
#include "lib/flanterm/backends/fb.h"
#include "lib/printf/printf.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "serial/serial.h"

#define LOG_FATAL_CLR "\033[31m"
#define LOG_ERROR_CLR "\033[91m"
#define LOG_WARN_CLR  "\033[93m"
#define LOG_INFO_CLR  "\033[92m"
#define LOG_DEBUG_CLR "\033[36m"
#define LOG_RESET_CLR "\033[37m"

static struct flanterm_context *ft_ctx;

static enum klog_lvl fb_lvl, serial_lvl;
static enum klog_lvl curr_print_lvl;

void _putchar(char c) {
    // we made flanterm_putchar available globally
    if (curr_print_lvl <= fb_lvl) {
        flanterm_putchar(ft_ctx, c);
    }

    if (curr_print_lvl <= serial_lvl) {
        serial_putchar(c);
    }
}

void klog_init(struct limine_framebuffer *fb, enum klog_lvl fb_log_lvl, enum klog_lvl serial_log_lvl) {
    fb_lvl = fb_log_lvl;
    serial_lvl = serial_log_lvl;

    if (fb_lvl != LOG_LVL_DISABLED) {
        ft_ctx = flanterm_fb_init(
            NULL,
            NULL,
            fb->address, fb->width, fb->height, fb->pitch,
            fb->red_mask_size, fb->red_mask_shift,
            fb->green_mask_size, fb->green_mask_shift,
            fb->blue_mask_size, fb->blue_mask_shift,
            NULL,
            NULL, NULL,
            NULL, NULL,
            NULL, NULL,
            NULL, 0, 0, 1,
            0, 0,
            0
        );
    }

    if (serial_lvl) {
        uint8_t serial_init_success = serial_init();
        if (!serial_init_success) {
            serial_lvl = LOG_LVL_DISABLED;
        }
    }
}

static const char *get_lvl_prefix(enum klog_lvl lvl) {
    switch (lvl) {
        case LOG_LVL_FATAL:
            return LOG_RESET_CLR "[" LOG_FATAL_CLR "FATAL" LOG_RESET_CLR "] ";
        case LOG_LVL_ERROR:
            return LOG_RESET_CLR "[" LOG_ERROR_CLR "ERROR" LOG_RESET_CLR "] ";
        case LOG_LVL_WARN:
            return LOG_RESET_CLR "[" LOG_WARN_CLR "WARN" LOG_RESET_CLR "] ";
        case LOG_LVL_INFO:
            return LOG_RESET_CLR "[" LOG_INFO_CLR "INFO" LOG_RESET_CLR "] ";
        case LOG_LVL_DEBUG:
            return LOG_RESET_CLR "[" LOG_DEBUG_CLR "DEBUG" LOG_RESET_CLR "] ";
        default:
            kpanic("Invalid log level");
    }
}

static int kvlog(enum klog_lvl lvl, const char *fmt, va_list va) {
    curr_print_lvl = lvl;

    printf_(get_lvl_prefix(lvl));

    int ret = vprintf_(fmt, va);

    // add newline
    _putchar('\n');

    if (fb_lvl != LOG_LVL_DISABLED && ft_ctx->autoflush) {
        ft_ctx->double_buffer_flush(ft_ctx);
    }

    return ret;    
}

int klog_fatal(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    const int ret = kvlog(LOG_LVL_FATAL, fmt, va);
    va_end(va);
    return ret;
}

int klog_error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    const int ret = kvlog(LOG_LVL_ERROR, fmt, va);
    va_end(va);
    return ret;
}

int klog_warn(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    const int ret = kvlog(LOG_LVL_WARN, fmt, va);
    va_end(va);
    return ret;
}

int klog_info(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    const int ret = kvlog(LOG_LVL_INFO, fmt, va);
    va_end(va);
    return ret;
}

int klog_debug(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    const int ret = kvlog(LOG_LVL_DEBUG, fmt, va);
    va_end(va);
    return ret;
}
