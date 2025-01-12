#include <stdbool.h>
#include <stddef.h>

#include "acpi/acpi.h"
#include "acpi/hpet.h"
#include "acpi/madt.h"
#include "arch/x86_64/apic/ioapic.h"
#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "arch/x86_64/pit/pit.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "limine.h"
#include "memory/kheap/kheap.h"
#include "memory/pmm/pmm.h"
#include "memory/vmm/vmm.h"
#include "timer/timer.h"

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

    klog_init(fb_request.response->framebuffers[0], LOG_LVL_INFO, LOG_LVL_DEBUG);

    gdt_init();
    interrupts_init();
    vmm_set_hhdm_offset(hhdm_offset);
    pmm_init(memmap);
    pmm_print_memmap(memmap);
    vmm_init(memmap, kaddr);
    kheap_init();
    acpi_init(rsdp->address);

    lapic_init();
    madt_init();
    ioapic_init();

    enable_interrupts();

    timer_init();

    lapic_timer_calibrate();

    kpanic("End of kmain");
}
