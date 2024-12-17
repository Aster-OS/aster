#include <stddef.h>

#include "kpanic/kpanic.h"
#include "kprintf/kprintf.h"
#include "lib/align.h"
#include "memory/kheap/kheap.h"
#include "memory/pmm/pmm.h"
#include "memory/vmm/vmm.h"

#define HEAP_START 0xffffffffd0000000
#define HEAP_SIZE 0x200000 // 2MiB
#define HEAP_END (HEAP_START + HEAP_SIZE)

#define HEAP_ALLOC_ALIGNMENT 8

static inline size_t heap_align_size(size_t size) {
    return (size_t) align_up(size, HEAP_ALLOC_ALIGNMENT);
}

struct alloc_header_t {
    size_t size;
};

struct freelist_node_t {
    size_t size;
    struct freelist_node_t *prev;
    struct freelist_node_t *next;
};

static struct freelist_node_t *freelist_head;

static void freelist_add_node(struct freelist_node_t *node_to_add) {
    if (freelist_head == NULL) {
        freelist_head = node_to_add;
        freelist_head->prev = NULL;
        freelist_head->next = NULL;
        return;
    }

    node_to_add->prev = NULL;
    node_to_add->next = freelist_head;
    freelist_head->prev = node_to_add;
    freelist_head = node_to_add;
}

static void freelist_remove_node(struct freelist_node_t *node_to_remove) {
    if (node_to_remove == NULL) {
        kpanic("Kheap attempted to remove NULL node\n");
        return;
    }

    if (node_to_remove->prev != NULL) {
        node_to_remove->prev->next = node_to_remove->next;
    }

    if (node_to_remove->next != NULL) {
        node_to_remove->next->prev = node_to_remove->prev;
    }

    if (node_to_remove == freelist_head) {
        freelist_head = freelist_head->next;
    }
}

static bool freelist_contains_addr(uintptr_t addr) {
    struct freelist_node_t *free_chunk = freelist_head;
    while (free_chunk != NULL) {
        uintptr_t chunk_addr = (uintptr_t) free_chunk;
        if (addr >= chunk_addr && addr < chunk_addr + free_chunk->size) {
            return true;
        }
        free_chunk = free_chunk->next;
    }
    return false;
}

void *kheap_alloc(size_t size) {
    size_t alloc_size = size + sizeof(struct alloc_header_t);
    alloc_size = heap_align_size(alloc_size);
    // make sure that after the allocation, when this chunk is freed,
    // it will be large enough to hold the freelist node struct in itself
    if (alloc_size < heap_align_size(sizeof(struct freelist_node_t))) {
        alloc_size = heap_align_size(sizeof(struct freelist_node_t));
    }

    // search for a free chunk which either has a free size
    //   1. equal to alloc_size => we remove the node, place the header and return the memory
    // or a free size
    //   2. of at least alloc_size + sizeof(struct node_t),
    //      so that we can split it, and after splitting, it will still be large enough
    //      to hold the freelist node struct in itself
    
    struct freelist_node_t *free_chunk = freelist_head;
    while (free_chunk != NULL) {
        if (free_chunk->size == alloc_size ||
            free_chunk->size >= alloc_size + sizeof(struct freelist_node_t)) {
            break;
        }
        free_chunk = free_chunk->next;
    }

    if (free_chunk == NULL) {
        kpanic("Kheap out of memory\n");
        return NULL;
    }

    if (free_chunk->size == alloc_size) {
        freelist_remove_node(free_chunk);
    } else {
        // free_chunk->size >= alloc_size + sizeof(struct node_t)
        struct freelist_node_t *remaining_free_chunk = (struct freelist_node_t *) ((uintptr_t) free_chunk + alloc_size);
        remaining_free_chunk->size = free_chunk->size - alloc_size;
        freelist_add_node(remaining_free_chunk);
        freelist_remove_node(free_chunk);
    }

    struct alloc_header_t *allocated_chunk_hdr = (struct alloc_header_t *) free_chunk;
    allocated_chunk_hdr->size = alloc_size;

    kprintf("alloc %016llx alloc_size 0x%x\n", (uintptr_t) (allocated_chunk_hdr + 1), alloc_size);

    return (void *) (allocated_chunk_hdr + 1);
}

void kheap_free(void *ptr) {
    if (freelist_contains_addr((uintptr_t) ptr)) {
        kprintf("Kheap attempted to free already free address!\n");
        return;
    }

    if ((uintptr_t) ptr < HEAP_START || (uintptr_t) ptr >= HEAP_END) {
        kpanic("Invalid free pointer\n");
    }

    struct alloc_header_t *header = (struct alloc_header_t *) ptr - 1;
    size_t freed_size = header->size;

    struct freelist_node_t *freed_chunk_node = (struct freelist_node_t *) header;
    freed_chunk_node->size = freed_size;
    freelist_add_node(freed_chunk_node);

    kprintf("free %016llx\n", (uintptr_t) ptr);
}

void kheap_init(void) {
    for (uintptr_t virt = HEAP_START; virt < HEAP_END; virt += PAGE_SIZE) {
        phys_t phys = pmm_alloc(true);
        vmm_map_page(vmm_get_kernel_pagemap(), virt, phys, PTE_FLAG_WRITE | PTE_FLAG_NX);
    }

    struct freelist_node_t *head = (struct freelist_node_t *) HEAP_START;
    head->size = HEAP_SIZE;
    freelist_add_node(head);

    kprintf("Heap initialized with %dMiB of avl. memory\n", HEAP_SIZE >> 20);
}
