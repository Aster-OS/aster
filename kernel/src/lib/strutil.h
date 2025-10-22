#pragma once

#include <stddef.h>

#define strcmp kstrcmp

int kstrcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
int strncmp(const char *s1, const char *s2, size_t n);
