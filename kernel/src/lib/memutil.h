#pragma once

#include <stddef.h>

#define kmemcmp  memcmp
#define kmemcpy  memcpy
#define kmemmove memmove
#define kmemset  memset

int memcmp(void const *s1, void const *s2, size_t n);
void *memcpy(void *dest, void const *src, size_t n);
void *memmove(void *dest, void const *src, size_t n);
void *memset(void *s, int c, size_t n);
