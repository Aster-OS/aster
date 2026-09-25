#include "fs/initrd.h"

#include <stddef.h>

#include "fs/ustar.h"

static void *initrd_address;
static size_t initrd_size;

void initrd_init(void *address, size_t size) {
    initrd_address = address;
    initrd_size = size;
}

void initrd_open(char const *path, struct ustar_file_t *file) {
    ustar_open(initrd_address, initrd_size, path, file);
}
