#pragma once

#include <stddef.h>

void *kmalloc(size_t sz);
void kmalloc_init(void);
void kfree(void *ptr);
