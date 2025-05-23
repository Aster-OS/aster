#include "acpi/acpi.h"
#include "acpi/madt.h"
#include "arch/x86_64/apic/ioapic.h"
#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/idt/idt.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "arch/x86_64/pit/pit.h"
#include "dev/tty/debugcon.h"
#include "dev/tty/flanterm.h"
#include "dev/tty/serial.h"
#include "kassert/kassert.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/elf/symbols.h"
#include "limine.h"
#include "memory/kmalloc/kmalloc.h"
#include "memory/pmm/pmm.h"
#include "memory/vmm/vmm.h"
#include "mp/mp.h"
#include "sched/sched.h"
#include "timer/timer.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3)

__attribute__((used, section(".limine_requests")))
static volatile struct limine_bootloader_info_request bootloader_info_request = {
    .id = LIMINE_BOOTLOADER_INFO_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request executable_addr_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_file_request executable_file_request = {
    .id = LIMINE_EXECUTABLE_FILE_REQUEST,
    .revision = 0
};

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
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_mp_request mp_request = {
    .id = LIMINE_MP_REQUEST,
    .flags = LIMINE_MP_X2APIC,
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

static void *test_thread(void *arg) {
    for (uint64_t i = 0; i < (uint64_t) arg; i++) {
        if (i % 1000 == 0) {
            klog_debug("Thread %03llu running on CPU %llu", get_cpu()->curr_thread->tid, get_cpu()->id);
        }
    }

    return NULL;
}

void kernel_entry(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        halt();
    }

    if (fb_request.response == NULL || fb_request.response->framebuffer_count < 1) {
        halt();
    }

    struct limine_bootloader_info_response *bootloader_info = bootloader_info_request.response;
    struct limine_executable_address_response *executable_addr = executable_addr_request.response;
    struct limine_executable_file_response *executable_file = executable_file_request.response;
    struct limine_framebuffer_response *fb = fb_request.response;
    struct limine_hhdm_response *hhdm = hhdm_request.response;
    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_mp_response *mp = mp_request.response;
    struct limine_rsdp_response *rsdp = rsdp_request.response;

    mp_init_bsp(mp);
    cpuid_init();

    struct tty_t *flanterm_tty = flanterm_tty_init(fb->framebuffers[0]);
    flanterm_tty->lvl = KLOG_LVL_INFO;
    klog_register_tty(flanterm_tty);

    struct tty_t *serial_tty = serial_tty_init();
    if (serial_tty != NULL) {
        serial_tty->lvl = KLOG_LVL_DEBUG;
        klog_register_tty(serial_tty);
    }

    struct tty_t *debugcon_tty = debugcon_tty_init();
    if (debugcon_tty != NULL) {
        debugcon_tty->lvl = KLOG_LVL_DEBUG;
        klog_register_tty(debugcon_tty);
    }

    klog_info("Aster booted by %s v%s", bootloader_info->name, bootloader_info->version);
    klog_info("Built at commit %s", COMMIT_HASH);

    char cpu_brand_str[48];
    if (cpu_get_brand_str(cpu_brand_str)) {;
        klog_info("Running on %.48s", cpu_brand_str);
    }

    kassert(bootloader_info != NULL);
    kassert(executable_addr != NULL);
    kassert(executable_file != NULL);
    // framebuffer check is done above
    kassert(hhdm != 0);
    kassert(memmap != NULL);
    kassert(mp != NULL);
    kassert(rsdp != NULL);

    gdt_init();
    gdt_reload_segments();
    gdt_reload_tss();
    idt_init();
    idt_reload();
    interrupts_init();
    vmm_set_hhdm_offset(hhdm->offset);
    pmm_init(memmap);
    pmm_print_memmap(memmap);
    vmm_init(memmap, executable_addr);
    kmalloc_init();
    symbols_init(executable_file->executable_file->address);
    acpi_init(rsdp->address);
    madt_init();
    lapic_init();
    ioapic_init();
    cpu_set_int_state(true);
    timer_init();
    lapic_timer_calibrate();
    sched_init();
    sched_init_cpu();
    mp_init(mp);

    for (size_t i = 0; i < 10; i++) {
        sched_new_kthread(test_thread, (void *) (uint64_t) 500000);
        sched_new_kthread(test_thread, (void *) (uint64_t) 400000);
        sched_new_kthread(test_thread, (void *) (uint64_t) 300000);
        sched_new_kthread(test_thread, (void *) (uint64_t) 200000);
        sched_new_kthread(test_thread, (void *) (uint64_t) 100000);
    }

    sched_yield();
}
