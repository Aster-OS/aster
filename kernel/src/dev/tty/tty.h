#pragma once

#include <stdbool.h>

#include "klog/klog_lvl.h"

struct tty_t {
    enum klog_lvl lvl;
    bool do_flush;
    void (*flush)(void);
    void (*putchar)(char c);
};
