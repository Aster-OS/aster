#pragma once

#include <stddef.h>

#include "fs/ustar.h"

void initrd_init(void *address, size_t size);
void initrd_open(char const *path, struct ustar_file_t *file);
