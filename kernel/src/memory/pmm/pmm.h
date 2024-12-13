#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "limine.h"

#define PAGE_SIZE 4096

typedef uint64_t phys_t;

void pmm_init(struct limine_memmap_response *memmap);
phys_t pmm_alloc(bool zero_contents);
phys_t pmm_alloc_n(uint64_t n_pages, bool zero_contents);
void pmm_free(phys_t addr);
void pmm_free_n(phys_t addr, uint64_t n_pages);
void pmm_print_memmap(struct limine_memmap_response *memmap);
