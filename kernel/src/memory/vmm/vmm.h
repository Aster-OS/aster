#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "limine.h"
#include "memory/pmm/pmm.h"

#define VMM_WRITE (1 << 0)
#define VMM_EXEC  (1 << 1)
#define VMM_USER  (1 << 2)

#define VMM_LOWER_HALF_MAX  0x00007fffffffffff
#define VMM_HIGHER_HALF_MIN 0xffff800000000000

struct vmm_range_t {
    uintptr_t base;
    uintptr_t length;
    bool is_free;
};

struct pagemap_t {
    phys_t root;
};

typedef uint64_t vmm_flags_t;

void *vmm_alloc(struct pagemap_t *pagemap, uintptr_t size, vmm_flags_t flags);
void vmm_init(struct limine_memmap_response *memmap,
              struct limine_executable_address_response *executable_addr);
uintptr_t vmm_hhdm(void);
void vmm_load_pagemap(struct pagemap_t *pagemap);
struct pagemap_t *vmm_kernel_pagemap(void);
void vmm_map_page(struct pagemap_t *pagemap, uintptr_t vaddr, phys_t paddr,
                  vmm_flags_t flags);
void vmm_map_hhdm(phys_t phys);
struct pagemap_t *vmm_new_user_pagemap(void);
void vmm_set_hhdm_offset(uintptr_t offset);
void vmm_unmap_page(struct pagemap_t *pagemap, uintptr_t vaddr, bool free);
phys_t vmm_walk(struct pagemap_t *pagemap, uintptr_t vaddr);
