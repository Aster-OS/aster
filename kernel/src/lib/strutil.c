#include "lib/strutil.h"

#include <stddef.h>

int strcmp(char const *s1, char const *s2) {
    unsigned char const *p1 = (unsigned char const *) s1;
    unsigned char const *p2 = (unsigned char const *) s2;

    while (*p1 && *p1 == *p2) {
        ++p1;
        ++p2;
    }

    return (*p1 > *p2) - (*p2 > *p1);
}

int strncmp(char const *s1, char const *s2, size_t n) {
    unsigned char const *p1 = (unsigned char const *) s1;
    unsigned char const *p2 = (unsigned char const *) s2;

    while (n && *p1 && (*p1 == *p2)) {
        ++p1;
        ++p2;
        --n;
    }

    if (n == 0) {
        return 0;
    } else {
        return (*p1 - *p2);
    }
}

size_t strlen(char const *s) {
    if (!s) {
        return 0;
    }

    unsigned char const *str = (unsigned char const *) s;
    unsigned char const *ptr = (unsigned char const *) s;
    while (*ptr) {
        ptr++;
    }

    return ptr - str;
}

size_t strnlen(char const *s, size_t maxlen) {
    size_t len = 0;
    while (len < maxlen && s[len] != 0) {
        len++;
    }
    return len;
}
