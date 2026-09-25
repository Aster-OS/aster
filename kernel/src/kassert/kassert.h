#pragma once

#define kassert(assertion)                                                     \
    (assertion) ? (void) 0                                                     \
                : kassert_fail(#assertion, __FILE__, __LINE__, __func__)

void kassert_fail(char const *assertion_str, char const *file, int line,
                  char const *func);
