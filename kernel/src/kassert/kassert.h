#pragma once

// TODO provide an option to disable kernel asserts
#define kassert(assertion) \
    (assertion) ? (void) 0 : kassert_fail(#assertion, __FILE__, __LINE__, __func__)

void kassert_fail(const char *assertion_str, const char *file, int line, const char *func);
