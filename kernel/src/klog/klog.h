#pragma once

#include "dev/tty/tty.h"
#include "klog/klog_lvl.h"

void klog(enum klog_lvl lvl, ...);
void klog_register_tty(struct tty_t *tty);

#define klog_fatal(...) klog(KLOG_LVL_FATAL, ##__VA_ARGS__)
#define klog_error(...) klog(KLOG_LVL_ERROR, ##__VA_ARGS__)
#define klog_warn(...)  klog(KLOG_LVL_WARN, ##__VA_ARGS__)
#define klog_info(...)  klog(KLOG_LVL_INFO, ##__VA_ARGS__)
#define klog_debug(...) klog(KLOG_LVL_DEBUG, ##__VA_ARGS__)
