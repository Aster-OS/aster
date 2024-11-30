#include <stddef.h>

#include "kprintf/kprintf.h"
#include "lib/align.h"
#include "memory/pmm/pmm.h"
#include "memory/vmm/vmm.h"

#define PTE_FLAG_PRESENT (1 << 0)
#define PTE_FLAG_WRITE (1 << 1)
#define PTE_FLAG_USER (1 << 2)
#define PTE_FLAG_NX (1ull << 63)
#define PTE_PHYS_ADDR_MASK 0x7fffffffff000

extern uint64_t hhdm_offset;
extern struct limine_memmap_response *memmap;
extern struct limine_kernel_address_response *kaddr;

extern unsigned char __TEXT_START[], __TEXT_END[];
extern unsigned char __RODATA_START[], __RODATA_END[];
extern unsigned char __DATA_START[], __DATA_END[];
extern unsigned char __LIMINE_REQUESTS_START[], __LIMINE_REQUESTS_END[];

static uint64_t kernel_pagemap;

static uint64_t vmm_get_next_pml(uint64_t pml, uint64_t pml_index) {
    uint64_t *pml_hhdm = (uint64_t *) (pml + hhdm_offset);
    uint64_t *pml_entry = &pml_hhdm[pml_index];

    if (*pml_entry & PTE_FLAG_PRESENT) {
        return *pml_entry & PTE_PHYS_ADDR_MASK;
    }

    // the requested flags will be set only for the pml1 entry,
    // allowing pages with different permissions at the last level
    // all other pml entries are granted all permissions (write, user, execute)
    *pml_entry = (uint64_t) pmm_alloc(true) | PTE_FLAG_PRESENT | PTE_FLAG_WRITE | PTE_FLAG_USER;
    return *pml_entry & PTE_PHYS_ADDR_MASK;
}

void vmm_map_page(uint64_t pagemap, uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    uint64_t pml4_index = (virtual_address >> 39) & 0x1ff;
    uint64_t pml3_index = (virtual_address >> 30) & 0x1ff;
    uint64_t pml2_index = (virtual_address >> 21) & 0x1ff;
    uint64_t pml1_index = (virtual_address >> 12) & 0x1ff;

    uint64_t pml4 = pagemap;
    uint64_t pml3 = vmm_get_next_pml(pml4, pml4_index);
    uint64_t pml2 = vmm_get_next_pml(pml3, pml3_index);
    uint64_t pml1 = vmm_get_next_pml(pml2, pml2_index);

    uint64_t *pml1_hhdm = (uint64_t *) (pml1 + hhdm_offset);
    // pages are always mapped with the present flag set
    pml1_hhdm[pml1_index] = physical_address | PTE_FLAG_PRESENT | flags;
}

void vmm_init(void) {
    kernel_pagemap = (uint64_t) pmm_alloc(true);

    uint64_t text_start = (uint64_t) &__TEXT_START;
    uint64_t text_end   = (uint64_t) &__TEXT_END;
    uint64_t rodata_start = (uint64_t) &__RODATA_START;
    uint64_t rodata_end   = (uint64_t) &__RODATA_END;
    uint64_t data_start = (uint64_t) &__DATA_START;
    uint64_t data_end   = (uint64_t) &__DATA_END;
    uint64_t limine_reqs_start = (uint64_t) &__LIMINE_REQUESTS_START;
    uint64_t limine_reqs_end = (uint64_t) &__LIMINE_REQUESTS_END;

    for (uint64_t i = text_start; i < text_end; i += PAGE_SIZE) {
        uint64_t virt = i;
        uint64_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        vmm_map_page(kernel_pagemap, virt, phys, 0);
    }

    for (uint64_t i = rodata_start; i < rodata_end; i += PAGE_SIZE) {
        uint64_t virt = i;
        uint64_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        vmm_map_page(kernel_pagemap, virt, phys, PTE_FLAG_NX);
    }

    for (uint64_t i = data_start; i < data_end; i += PAGE_SIZE) {
        uint64_t virt = i;
        uint64_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        vmm_map_page(kernel_pagemap, virt, phys, PTE_FLAG_WRITE | PTE_FLAG_NX);
    }

    for (uint64_t i = limine_reqs_start; i < limine_reqs_end; i += PAGE_SIZE) {
        uint64_t virt = i;
        uint64_t phys = i - kaddr->virtual_base + kaddr->physical_base;
        vmm_map_page(kernel_pagemap, virt, phys, PTE_FLAG_NX);
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        // map only usable, bootloader recl, kernel/modules and framebuffer entries
        // as per Limine base revision 3
        if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
        || entry->type == LIMINE_MEMMAP_KERNEL_AND_MODULES || entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            uint64_t entry_start = ALIGN_DOWN(entry->base, PAGE_SIZE);
            uint64_t entry_end = ALIGN_UP(entry->base + entry->length, PAGE_SIZE);

            for (uint64_t j = entry_start; j < entry_end; j += PAGE_SIZE) {
                vmm_map_page(kernel_pagemap, j + hhdm_offset, j, PTE_FLAG_WRITE | PTE_FLAG_NX);
            }
        }
    }

    vmm_load_kernel_pagemap();

    kprintf("VMM initialized\n");
}

void vmm_load_kernel_pagemap(void) {
    __asm__ volatile("mov %0, %%cr3" : : "r" (kernel_pagemap) : "memory");
}