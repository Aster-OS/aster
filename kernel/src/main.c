#include <stdbool.h>
#include <stddef.h>

#include "acpi/acpi.h"
#include "acpi/madt.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/idt/idt.h"
#include "kpanic/kpanic.h"
#include "kprintf/kprintf.h"
#include "limine.h"
#include "memory/kheap/kheap.h"
#include "memory/pmm/pmm.h"
#include "memory/vmm/vmm.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3)

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_kernel_address_request kaddr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER

static void halt(void) {
    __asm__ volatile("cli; hlt");
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        halt();
    }

    if (fb_request.response == NULL || fb_request.response->framebuffer_count < 1) {
        halt();
    }

    uint64_t hhdm_offset = hhdm_request.response->offset;
    struct limine_kernel_address_response *kaddr = kaddr_request.response;
    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_rsdp_response *rsdp = rsdp_request.response;

    kprintf_init(fb_request.response->framebuffers[0]);
    kprintf("Hello, %s!\n", "World");

    gdt_init();
    idt_init();

    vmm_set_hhdm_offset(hhdm_offset);
    pmm_init(memmap);
    vmm_init(memmap, kaddr);
    kheap_init();
    acpi_init(rsdp->address);
    madt_init();

    kpanic("End of kmain\n");
}
