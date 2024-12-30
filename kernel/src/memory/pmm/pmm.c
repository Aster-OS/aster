#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/align.h"
#include "lib/bitmap/bitmap.h"
#include "memory/pmm/pmm.h"
#include "memory/vmm/vmm.h"

static struct bitmap_t bitmap;
static phys_t page_allocation_start;

void pmm_init(struct limine_memmap_response *memmap) {
    struct limine_memmap_entry *largest_usable_entry = memmap->entries[0];

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length > largest_usable_entry->length) {
            largest_usable_entry = entry;
        }
    }

    uint64_t usable_pages = largest_usable_entry->length / PAGE_SIZE;
    
    // how many pages are needed to store the bitmap?
    // each bit  stores the state of an usable page  |*8
    // each byte stores the state of  8 usable pages |*PAGE_SIZE
    // each page stores the state of (8 * PAGE_SIZE) usable pages
    //   N pages store  the state of    USABLE_PAGES usable pages
    // N = USABLE_PAGES / (8*PAGE_SIZE) [aligned up!]
    uint64_t pages_used_to_store_bitmap = div_and_align_up(usable_pages, 8 * PAGE_SIZE);

    // start allocating physical memory after the bitmap
    page_allocation_start
        = (phys_t) (largest_usable_entry->base + pages_used_to_store_bitmap * PAGE_SIZE);

    bitmap.start = (uint8_t *) (largest_usable_entry->base + vmm_get_hhdm_offset());
    bitmap.bit_count = usable_pages - pages_used_to_store_bitmap;
    for (uint64_t i = 0; i < pages_used_to_store_bitmap * PAGE_SIZE; i++) {
        bitmap.start[i] = 0;
    }

    klog_info("PMM initialized with %dMiB of avl. phys. mem.", largest_usable_entry->length >> 20);
}

phys_t pmm_alloc(bool zero_contents) {
    uint64_t page_index = 0;
    while (page_index < bitmap.bit_count && bitmap_get_bit(&bitmap, page_index)) {
        page_index++;
    }

    if (page_index == bitmap.bit_count) {
        kpanic("Out of memory");
    } else {
        bitmap_set_bit(&bitmap, page_index);

        phys_t page_addr = page_allocation_start + page_index * PAGE_SIZE;

        if (!zero_contents) {
            return page_addr;
        }

        uint8_t *page_start = (uint8_t *) (page_addr + vmm_get_hhdm_offset());
        uint8_t *page_end = (uint8_t *) (page_start + PAGE_SIZE);
        for (uint8_t *i = page_start; i < page_end; i++) {
            *i = 0;
        }

        return page_addr;
    }
}

phys_t pmm_alloc_n(uint64_t n_pages, bool zero_contents) {
    bool found_n_pages = false;
    uint64_t page_index = 0;
    while (page_index < bitmap.bit_count - n_pages + 1) {
        if (!bitmap_get_bit(&bitmap, page_index)) {
            found_n_pages = true;
            
            for (uint64_t i = page_index; i < page_index + n_pages; i++) {
                if (bitmap_get_bit(&bitmap, i)) {
                    found_n_pages = false;
                    break;
                }
            }

            if (found_n_pages) {
                break;
            }
        }

        page_index++;
    }

    if (!found_n_pages) {
        kpanic("Out of memory");
    } else {
        for (uint64_t i = page_index; i < page_index + n_pages; i++) {
            bitmap_set_bit(&bitmap, i);
        }

        phys_t page_addr = page_allocation_start + page_index * PAGE_SIZE;

        if (!zero_contents) {
            return page_addr;
        }

        uint8_t *page_start = (uint8_t *) (page_addr + vmm_get_hhdm_offset());
        uint8_t *page_end = (uint8_t *) (page_start + n_pages * PAGE_SIZE);
        for (uint8_t *i = page_start; i < page_end; i++) {
            *i = 0;
        }

        return page_addr;
    }
}

void pmm_free(phys_t addr) {
    uint64_t page_index = (addr - page_allocation_start) / PAGE_SIZE;
    bitmap_unset_bit(&bitmap, page_index);
}

void pmm_free_n(phys_t addr, uint64_t n_pages) {
    uint64_t page_index = (addr - page_allocation_start) / PAGE_SIZE;
    for (uint64_t i = page_index; i < page_index + n_pages; i++) {
        bitmap_unset_bit(&bitmap, i);
    }
}

static char *get_entry_type(uint64_t entry_type) {
    switch (entry_type) {
        case LIMINE_MEMMAP_USABLE:
            return "Usable";
        case LIMINE_MEMMAP_RESERVED:
            return "Reserved";
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            return "ACPI Recl.";
        case LIMINE_MEMMAP_ACPI_NVS:
            return "ACPI NVS";
        case LIMINE_MEMMAP_BAD_MEMORY:
            return "Bad Memory";
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            return "Bootloader Recl.";
        case LIMINE_MEMMAP_KERNEL_AND_MODULES:
            return "Kernel & Modules";
        case LIMINE_MEMMAP_FRAMEBUFFER:
            return "Framebuffer";
        default:
            return "Unknown";
    }
}

void pmm_print_memmap(struct limine_memmap_response *memmap) {
    klog_debug("Physical memory layout:");
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        phys_t start = entry->base;
        phys_t end = entry->base + entry->length;
        uint64_t length_in_mib = entry->length >> 20;
        char *type = get_entry_type(entry->type);
        klog_debug("%016llx - %016llx %5d MiB %s", start, end, length_in_mib, type);
    }
}
