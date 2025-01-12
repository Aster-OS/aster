#pragma once

#include <stddef.h>

void *kheap_alloc(size_t sz);
void kheap_free(void *ptr);
void kheap_init(void);
