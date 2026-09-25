#pragma once

#include <stddef.h>
#include <stdint.h>

struct ustar_file_t {
    void *address;
    uint64_t size;
};

void ustar_open(void *addr, size_t size, char const *path,
                struct ustar_file_t *file);
