#pragma once

#define kassert(assertion) \
    kassert_internal(assertion, #assertion, __FILE__, __LINE__, __func__)

void kassert_internal(int assertion, const char *assertion_str,
                        const char *file, int line, const char *func);
