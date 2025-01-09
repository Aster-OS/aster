#pragma once

#include <stdarg.h>

#include "limine.h"

enum klog_lvl {
    LOG_LVL_DISABLED = 0,
    LOG_LVL_FATAL = 1,
    LOG_LVL_ERROR = 2,
    LOG_LVL_WARN = 3,
    LOG_LVL_INFO = 4,
    LOG_LVL_DEBUG = 5
};

void klog_init(struct limine_framebuffer *fb, enum klog_lvl fb_log_lvl, enum klog_lvl serial_log_lvl);

int klog_fatal(const char *fmt, ...);
int klog_error(const char *fmt, ...);
int klog_warn(const char *fmt, ...);
int klog_info(const char *fmt, ...);
int klog_debug(const char *fmt, ...);
