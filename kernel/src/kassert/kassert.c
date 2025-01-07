#include "lib/printf/printf.h"
#include "kassert/kassert.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"

static char kassert_buf[256];

void kassert_internal(int assertion, const char *assertion_str, const char *file, int line, const char *func) {
    if (assertion == 0) {
        klog_error("Assertion failed: %s", assertion_str);
        klog_error("  in function %s", func);
        klog_error("  in file %s:%d", file, line);

        kpanic("Assertion failed");
    }
}
