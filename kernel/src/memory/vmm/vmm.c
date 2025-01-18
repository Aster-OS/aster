#include "arch/x86_64/asm_wrappers.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/align.h"
#include "memory/pmm/pmm.h"
#include "memory/vmm/vmm.h"

static const uint64_t VMM_PAGE_PRESENT = 1 << 0;
static const uint64_t VMM_FLAGS_HHDM = VMM_PAGE_WRITE | VMM_PAGE_NX;

typedef uint64_t pml_entry_t;
static const uint64_t PTE_PHYS_ADDR_MASK = 0x7fffffffff000;

extern unsigned char __TEXT_START[], __TEXT_END[];
extern unsigned char __RODATA_START[], __RODATA_END[];
extern unsigned char __DATA_START[], __DATA_END[];
extern unsigned char __LIMINE_REQUESTS_START[], __LIMINE_REQUESTS_END[];

static uintptr_t hhdm_offset;

static phys_t kernel_pagemap;

void vmm_init(struct limine_memmap_response *memmap, struct limine_executable_address_response *executable_addr) {
    kernel_pagemap = pmm_alloc(true);

    uintptr_t text_start = (uintptr_t) &__TEXT_START;
    uintptr_t text_end   = (uintptr_t) &__TEXT_END;
    uintptr_t rodata_start = (uintptr_t) &__RODATA_START;
    uintptr_t rodata_end   = (uintptr_t) &__RODATA_END;
    uintptr_t data_start = (uintptr_t) &__DATA_START;
    uintptr_t data_end   = (uintptr_t) &__DATA_END;
    uintptr_t limine_reqs_start = (uintptr_t) &__LIMINE_REQUESTS_START;
    uintptr_t limine_reqs_end = (uintptr_t) &__LIMINE_REQUESTS_END;

    uintptr_t virt_to_phys_slide = executable_addr->virtual_base - executable_addr->physical_base;

    for (uintptr_t i = text_start; i < text_end; i += PAGE_SIZE) {
        uintptr_t virt = i;
        phys_t phys = i - virt_to_phys_slide;
        vmm_map_page(kernel_pagemap, virt, phys, 0);
    }

    for (uintptr_t i = rodata_start; i < rodata_end; i += PAGE_SIZE) {
        uintptr_t virt = i;
        phys_t phys = i - virt_to_phys_slide;
        vmm_map_page(kernel_pagemap, virt, phys, VMM_PAGE_NX);
    }

    for (uintptr_t i = data_start; i < data_end; i += PAGE_SIZE) {
        uintptr_t virt = i;
        phys_t phys = i - virt_to_phys_slide;
        vmm_map_page(kernel_pagemap, virt, phys, VMM_PAGE_WRITE | VMM_PAGE_NX);
    }

    for (uintptr_t i = limine_reqs_start; i < limine_reqs_end; i += PAGE_SIZE) {
        uintptr_t virt = i;
        phys_t phys = i - virt_to_phys_slide;
        vmm_map_page(kernel_pagemap, virt, phys, VMM_PAGE_NX);
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        // map only usable, bootloader recl, kernel/modules and framebuffer entries
        // as per Limine base revision 3
        if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
        || entry->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES || entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            uintptr_t entry_start = align_down(entry->base, PAGE_SIZE);
            uintptr_t entry_end = align_up(entry->base + entry->length, PAGE_SIZE);

            for (uintptr_t j = entry_start; j < entry_end; j += PAGE_SIZE) {
                vmm_map_page(kernel_pagemap, j + hhdm_offset, j, VMM_FLAGS_HHDM);
            }
        }
    }

    vmm_load_pagemap(kernel_pagemap);

    klog_info("VMM initialized");
}

uintptr_t vmm_get_hhdm_offset(void) {
    return hhdm_offset;
}

phys_t vmm_get_kernel_pagemap(void) {
    return kernel_pagemap;
}

void vmm_load_pagemap(phys_t pagemap) {
    wr_cr3(pagemap);
}

static phys_t get_next_pml(phys_t pml, uint16_t pml_index) {
    pml_entry_t *pml_hhdm = (pml_entry_t *) (pml + hhdm_offset);
    pml_entry_t *pml_entry = &pml_hhdm[pml_index];

    if (*pml_entry & VMM_PAGE_PRESENT) {
        return *pml_entry & PTE_PHYS_ADDR_MASK;
    }

    // the requested flags will be set only for the pml1 entry,
    // allowing pages with different permissions at the last level
    // all other pml entries are granted all permissions (write, user, execute)
    *pml_entry = (pml_entry_t) pmm_alloc(true) | VMM_PAGE_PRESENT | VMM_PAGE_WRITE | VMM_PAGE_USER;
    return *pml_entry & PTE_PHYS_ADDR_MASK;
}

static inline pml_entry_t *get_pml1_entry(phys_t pagemap, uintptr_t virt) {
    uint16_t pml4_index = (virt >> 39) & 0x1ff;
    uint16_t pml3_index = (virt >> 30) & 0x1ff;
    uint16_t pml2_index = (virt >> 21) & 0x1ff;
    uint16_t pml1_index = (virt >> 12) & 0x1ff;

    phys_t pml4 = pagemap;
    phys_t pml3 = get_next_pml(pml4, pml4_index);
    phys_t pml2 = get_next_pml(pml3, pml3_index);
    phys_t pml1 = get_next_pml(pml2, pml2_index);

    pml_entry_t *pml1_hhdm = (pml_entry_t *) (pml1 + hhdm_offset);
    pml_entry_t *pml1_entry = &pml1_hhdm[pml1_index];
    return pml1_entry;
}

static inline void invlpg_if_needed(phys_t pagemap, uintptr_t virt) {
    phys_t curr_pagemap = rd_cr3();
    if (pagemap == curr_pagemap) {
        invlpg(virt);
    }
}

void vmm_map_hhdm(phys_t phys) {
    vmm_map_page(kernel_pagemap, phys + hhdm_offset, phys, VMM_FLAGS_HHDM);
}

void vmm_map_page(phys_t pagemap, uintptr_t virt, phys_t phys, uint64_t flags) {
    // pages are always mapped with the present flag set
    *get_pml1_entry(pagemap, virt) = phys | VMM_PAGE_PRESENT | flags;
    invlpg_if_needed(pagemap, virt);
}

void vmm_map_range_contig(phys_t pagemap, uintptr_t virt_start, phys_t phys_start, uint64_t page_count, uint64_t flags) {
    uintptr_t length = page_count * PAGE_SIZE;
    for (uintptr_t i = 0; i < length; i += PAGE_SIZE) {
        vmm_map_page(pagemap, virt_start + i, phys_start + i, flags);
    }
}

void vmm_set_hhdm_offset(uintptr_t offset) {
    if (hhdm_offset != 0) {
        kpanic("HHDM offset set twice");
    }

    hhdm_offset = offset;
}

void vmm_unmap_page(phys_t pagemap, uintptr_t virt) {
    *get_pml1_entry(pagemap, virt) = 0;
    invlpg_if_needed(pagemap, virt);
}

void vmm_unmap_range_contig(phys_t pagemap, uintptr_t virt_start, uint64_t page_count) {
    uintptr_t length = page_count * PAGE_SIZE;
    for (uintptr_t i = 0; i < length; i += PAGE_SIZE) {
        vmm_unmap_page(pagemap, virt_start + i);
    }
}

phys_t vmm_walk_page(phys_t pagemap, uintptr_t virt) {
    pml_entry_t pml1_entry = *get_pml1_entry(pagemap, virt);
    if (pml1_entry & VMM_PAGE_PRESENT) {
        uint16_t page_offset = virt & 0xfff;
        return (pml1_entry & PTE_PHYS_ADDR_MASK) | page_offset;
    } else {
        return 0;
    }
}
