#include "kassert/kassert.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"

void kassert_fail(const char *assertion_str, const char *file, int line, const char *func) {
    klog_error("Assertion failed: %s", assertion_str);
    klog_error("  in function %s", func);
    klog_error("  in file %s:%d", file, line);

    kpanic("Assertion failed");
}
