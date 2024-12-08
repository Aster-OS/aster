#pragma once

#include "limine.h"

#define PTE_FLAG_PRESENT (1 << 0)
#define PTE_FLAG_WRITE (1 << 1)
#define PTE_FLAG_USER (1 << 2)
#define PTE_FLAG_NX (1ull << 63)
#define PTE_PHYS_ADDR_MASK 0x7fffffffff000

#define PTE_FLAGS_HHDM (PTE_FLAG_WRITE | PTE_FLAG_NX)

void vmm_init(struct limine_memmap_response *memmap, struct limine_kernel_address_response *kaddr);
uint64_t vmm_get_hhdm_offset(void);
uint64_t vmm_get_kernel_pagemap(void);
void vmm_load_pagemap(uint64_t pagemap);
void vmm_map_page(uint64_t pagemap, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_set_hhdm_offset(uint64_t offset);
void vmm_unmap_page(uint64_t pagemap, uint64_t virt);
uint64_t vmm_walk_page(uint64_t pagemap, uint64_t virt);
