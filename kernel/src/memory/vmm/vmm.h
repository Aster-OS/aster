#pragma once

#include "limine.h"

void vmm_init(void);
void vmm_load_kernel_pagemap(void);
void vmm_map_page(uint64_t pagemap, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap_page(uint64_t pagemap, uint64_t virt);
uint64_t vmm_walk_page(uint64_t pagemap, uint64_t virt);
