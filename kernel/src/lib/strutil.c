#include "lib/strutil.h"

int strcmp(const char *s1, const char *s2) {
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;

    while (*p1 && *p1 == *p2) {
        ++p1;
        ++p2;
    }

    return (*p1 > *p2) - (*p2 > *p1);
}

int strncmp(const char *s1, const char *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;

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
