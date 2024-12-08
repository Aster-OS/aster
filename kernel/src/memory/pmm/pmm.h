#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "limine.h"

#define PAGE_SIZE 4096

void pmm_init(struct limine_memmap_response *memmap);
void *pmm_alloc(bool zero_contents);
void *pmm_alloc_n(uint64_t n_pages, bool zero_contents);
void pmm_free(void *addr);
void pmm_free_n(void *addr, uint64_t n_pages);
void pmm_print_memmap(struct limine_memmap_response *memmap);
