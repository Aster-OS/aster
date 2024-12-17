#pragma once

#include <stdint.h>

void *kheap_alloc(size_t size);
void kheap_free(void *ptr);
void kheap_init(void);
