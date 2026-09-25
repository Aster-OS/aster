#pragma once

#include <stddef.h>

#define kstrcmp  strcmp
#define kstrlen  strlen
#define kstrncmp strncmp
#define kstrnlen strnlen

int strcmp(char const *s1, char const *s2);
size_t strlen(char const *s);
int strncmp(char const *s1, char const *s2, size_t n);
size_t strnlen(char const *s, size_t maxlen);
