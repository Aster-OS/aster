#include "kassert/kassert.h"

#include "klog/klog.h"
#include "kpanic/kpanic.h"

void kassert_fail(char const *assertion_str, char const *file, int line,
                  char const *func) {
    klog_error("Assertion failed: %s", assertion_str);
    klog_error("  in function %s", func);
    klog_error("  in file %s:%u", file, line);

    kpanic("Assertion failed");
}
