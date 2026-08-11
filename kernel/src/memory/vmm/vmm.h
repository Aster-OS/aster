#pragma once

#include <stdint.h>

#include "limine.h"
#include "memory/pmm/pmm.h"

#define VMM_WRITE (1 << 0)
#define VMM_EXEC  (1 << 1)
#define VMM_USER  (1 << 2)

typedef phys_t pagemap_t;

void vmm_init(struct limine_memmap_response *memmap,
              struct limine_executable_address_response *executable_addr);
uintptr_t vmm_hhdm_offset(void);
pagemap_t vmm_kernel_pagemap(void);
void vmm_map_page(pagemap_t pagemap, uintptr_t virt, phys_t phys,
                  uint64_t flags);
void vmm_map_hhdm(phys_t phys);
pagemap_t vmm_new_user_pagemap(void);
void vmm_set_hhdm_offset(uintptr_t offset);
void vmm_unmap_page(pagemap_t pagemap, uintptr_t virt);
