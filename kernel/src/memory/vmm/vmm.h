#pragma once

#include "limine.h"
#include "memory/pmm/pmm.h"

#define PTE_FLAG_WRITE (1 << 1)
#define PTE_FLAG_USER (1 << 2)
#define PTE_FLAG_NX (1ull << 63)
#define PTE_PHYS_ADDR_MASK 0x7fffffffff000

void vmm_init(struct limine_memmap_response *memmap, struct limine_executable_address_response *executable_addr);
uintptr_t vmm_get_hhdm_offset(void);
phys_t vmm_get_kernel_pagemap(void);
void vmm_load_pagemap(phys_t pagemap);
void vmm_map_hhdm(phys_t phys);
void vmm_map_page(phys_t pagemap, uintptr_t virt, phys_t phys, uint64_t flags);
void vmm_map_range_contig(phys_t pagemap, uintptr_t virt_start, phys_t phys_start, uint64_t page_count, uint64_t flags);
void vmm_set_hhdm_offset(uintptr_t offset);
void vmm_unmap_page(phys_t pagemap, uintptr_t virt);
void vmm_unmap_range_contig(phys_t pagemap, uintptr_t virt_start, uint64_t page_count);
phys_t vmm_walk_page(phys_t pagemap, uintptr_t virt);
