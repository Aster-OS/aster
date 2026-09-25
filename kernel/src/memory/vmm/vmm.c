#include "memory/vmm/vmm.h"

#include <stdbool.h>
#include <stdint.h>

#include "arch/x86_64/asm.h"
#include "kassert/kassert.h"
#include "klog/klog.h"
#include "lib/align.h"
#include "lib/compiler.h"
#include "lib/memutil.h"
#include "limine.h"
#include "memory/kmalloc/kmalloc.h"
#include "memory/pmm/pmm.h"
#include "mp/cpu.h"

#define VMM_HHDM VMM_WRITE

#define PML_PRESENT (1 << 0)
#define PML_WRITE   (1 << 1)
#define PML_USER    (1 << 2)
#define PML_GLOBAL  (1 << 8)
#define PML_NX      (1ULL << 63)

typedef uint64_t pml_entry_t;
typedef uint16_t pml_idx_t;

extern char LIMINE_REQUESTS_START;
extern char LIMINE_REQUESTS_END;
extern char TEXT_START;
extern char TEXT_END;
extern char RODATA_START;
extern char RODATA_END;
extern char DATA_START;
extern char DATA_END;

static uint64_t PML_ENTRY_PHYS_MASK;

static uintptr_t hhdm_offset;

static struct pagemap_t kernel_pagemap;

static inline ASTER_ALWAYS_INLINE pml_entry_t *
pml_entry_hhdm(phys_t pml, pml_idx_t pml_idx) {
    return (pml_entry_t *) (pml + pml_idx * sizeof(pml_entry_t) + hhdm_offset);
}

static pml_entry_t pml_entry_flags(vmm_flags_t vmm_flags) {
    pml_entry_t flags = PML_PRESENT;
    if (vmm_flags & VMM_WRITE) {
        flags |= PML_WRITE;
    }
    if ((vmm_flags & VMM_EXEC) == 0) {
        flags |= PML_NX;
    }
    if (vmm_flags & VMM_USER) {
        flags |= PML_USER;
    }
    return flags;
}

static phys_t next_pml_phys(phys_t pml, pml_idx_t pml_idx, bool alloc) {
    pml_entry_t *pml_entry = pml_entry_hhdm(pml, pml_idx);

    if (*pml_entry & PML_PRESENT) {
        return *pml_entry & PML_ENTRY_PHYS_MASK;
    }

    if (!alloc) {
        return 0;
    }

    *pml_entry =
        pmm_alloc(true) | pml_entry_flags(VMM_WRITE | VMM_EXEC | VMM_USER);
    return *pml_entry & PML_ENTRY_PHYS_MASK;
}

static inline ASTER_ALWAYS_INLINE pml_entry_t *
get_pml1_entry(struct pagemap_t *pagemap, uintptr_t vaddr) {
    pml_idx_t pml4_idx = (vaddr >> 39) & 0x1ff;
    pml_idx_t pml3_idx = (vaddr >> 30) & 0x1ff;
    pml_idx_t pml2_idx = (vaddr >> 21) & 0x1ff;
    pml_idx_t pml1_idx = (vaddr >> 12) & 0x1ff;

    phys_t pml4 = pagemap->root;
    phys_t pml3 = next_pml_phys(pml4, pml4_idx, true);
    phys_t pml2 = next_pml_phys(pml3, pml3_idx, true);
    phys_t pml1 = next_pml_phys(pml2, pml2_idx, true);

    return pml_entry_hhdm(pml1, pml1_idx);
}

static void invalidate_addr(struct pagemap_t *pagemap, uintptr_t vaddr) {
    if (pagemap->root == (rd_cr3() & PML_ENTRY_PHYS_MASK)) {
        invlpg(vaddr);
    }
}

static bool memmap_entry_in_hhdm(uint64_t type) {
    return type == LIMINE_MEMMAP_USABLE ||
           type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
           type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES ||
           type == LIMINE_MEMMAP_FRAMEBUFFER ||
           type == LIMINE_MEMMAP_ACPI_TABLES ||
           type == LIMINE_MEMMAP_ACPI_RECLAIMABLE ||
           type == LIMINE_MEMMAP_ACPI_NVS;
}

void vmm_init(struct limine_memmap_response *memmap,
              struct limine_executable_address_response *executable_addr) {
    kassert(hhdm_offset != 0);

    uint32_t eax, _;
    cpuid(0x80000008, 0x0, &eax, &_, &_, &_);
    uint32_t maxphyaddr = eax & 0xff;
    PML_ENTRY_PHYS_MASK = ((1ULL << maxphyaddr) - 1) & ~0xfff;

    kernel_pagemap.root = pmm_alloc(true);

    uintptr_t text_start = (uintptr_t) &TEXT_START;
    uintptr_t text_end = (uintptr_t) &TEXT_END;
    uintptr_t rodata_start = (uintptr_t) &RODATA_START;
    uintptr_t rodata_end = (uintptr_t) &RODATA_END;
    uintptr_t data_start = (uintptr_t) &DATA_START;
    uintptr_t data_end = (uintptr_t) &DATA_END;
    uintptr_t limine_reqs_start = (uintptr_t) &LIMINE_REQUESTS_START;
    uintptr_t limine_reqs_end = (uintptr_t) &LIMINE_REQUESTS_END;

    // Allocate higher half PML4 entries so they can be shared
    // in all address spaces
    for (pml_idx_t i = 256; i < 512; i++) {
        next_pml_phys(kernel_pagemap.root, i, true);
    }

    uintptr_t slide =
        executable_addr->virtual_base - executable_addr->physical_base;

    for (uintptr_t i = limine_reqs_start; i < limine_reqs_end; i += PAGE_SIZE) {
        vmm_map_page(&kernel_pagemap, i, i - slide, 0);
    }

    for (uintptr_t i = text_start; i < text_end; i += PAGE_SIZE) {
        vmm_map_page(&kernel_pagemap, i, i - slide, VMM_EXEC);
    }

    for (uintptr_t i = rodata_start; i < rodata_end; i += PAGE_SIZE) {
        vmm_map_page(&kernel_pagemap, i, i - slide, 0);
    }

    for (uintptr_t i = data_start; i < data_end; i += PAGE_SIZE) {
        vmm_map_page(&kernel_pagemap, i, i - slide, VMM_WRITE);
    }

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        if (!memmap_entry_in_hhdm(entry->type)) {
            continue;
        }

        uintptr_t entry_start = align_down(entry->base, PAGE_SIZE);
        uintptr_t entry_end = align_up(entry->base + entry->length, PAGE_SIZE);

        for (uintptr_t j = entry_start; j < entry_end; j += PAGE_SIZE) {
            vmm_map_page(&kernel_pagemap, j + hhdm_offset, j, VMM_HHDM);
        }
    }

    vmm_load_pagemap(&kernel_pagemap);

    klog_info("VMM initialized");
}

uintptr_t vmm_hhdm(void) {
    return hhdm_offset;
}

void vmm_load_pagemap(struct pagemap_t *pagemap) {
    wr_cr3(pagemap->root);
}

struct pagemap_t *vmm_kernel_pagemap(void) {
    return &kernel_pagemap;
}

void vmm_map_page(struct pagemap_t *pagemap, uintptr_t vaddr, phys_t paddr,
                  vmm_flags_t flags) {
    pml_entry_t *pml1_entry = get_pml1_entry(pagemap, vaddr);
    if (*pml1_entry != 0) {
        return;
    }
    *pml1_entry = (paddr & PML_ENTRY_PHYS_MASK) | pml_entry_flags(flags);
    invalidate_addr(pagemap, vaddr);
}

void vmm_map_hhdm(phys_t paddr) {
    vmm_map_page(&kernel_pagemap, paddr + hhdm_offset, paddr, VMM_HHDM);
}

struct pagemap_t *vmm_new_user_pagemap(void) {
    struct pagemap_t *pagemap = kmalloc(sizeof(struct pagemap_t));
    pagemap->root = pmm_alloc(true);

    uintptr_t entries_start = 256 * sizeof(pml_entry_t);
    uintptr_t entries_size = (512 - 256) * sizeof(pml_entry_t);
    void *dest = (void *) (pagemap->root + entries_start + hhdm_offset);
    void *src = (void *) (kernel_pagemap.root + entries_start + hhdm_offset);
    kmemcpy(dest, src, entries_size);

    return pagemap;
}

void vmm_set_hhdm_offset(uintptr_t offset) {
    hhdm_offset = offset;
}

void vmm_unmap_page(struct pagemap_t *pagemap, uintptr_t vaddr, bool free) {
    pml_entry_t *pml1_entry = get_pml1_entry(pagemap, vaddr);
    if (*pml1_entry & PML_PRESENT) {
        return;
    }
    if (free) {
        phys_t paddr = *pml1_entry & PML_ENTRY_PHYS_MASK;
        pmm_free(paddr);
    }
    *pml1_entry = 0;
    invalidate_addr(pagemap, vaddr);
}

phys_t vmm_walk(struct pagemap_t *pagemap, uintptr_t vaddr) {
    pml_entry_t *pml1_entry = get_pml1_entry(pagemap, vaddr);
    if (*pml1_entry & PML_PRESENT) {
        uint16_t page_offset = vaddr & 0xfff;
        return (*pml1_entry & PML_ENTRY_PHYS_MASK) | page_offset;
    } else {
        return 0;
    }
}
