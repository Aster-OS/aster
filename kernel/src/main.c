#include <stdbool.h>
#include <stddef.h>

#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/idt/idt.h"
#include "kpanic/kpanic.h"
#include "kprintf/kprintf.h"
#include "limine.h"
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

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER

static void halt(void) {
    __asm__ volatile("cli; hlt");
}

uint64_t hhdm_offset;
struct limine_kernel_address_response *kaddr;
struct limine_memmap_response *memmap;

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        halt();
    }

    if (fb_request.response == NULL || fb_request.response->framebuffer_count < 1) {
        halt();
    }

    hhdm_offset = hhdm_request.response->offset;
    kaddr = kaddr_request.response;
    memmap = memmap_request.response;

    kprintf_init(fb_request.response->framebuffers[0]);
    kprintf("Hello, %s!\n", "World");

    gdt_init();
    idt_init();
    pmm_init();
    vmm_init();

    kpanic("End of kmain");
}
