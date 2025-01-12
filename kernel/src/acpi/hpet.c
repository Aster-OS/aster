#include <stddef.h>

#include "acpi/acpi.h"
#include "acpi/hpet.h"
#include "arch/x86_64/asm_wrappers.h"
#include "klog/klog.h"
#include "memory/vmm/vmm.h"

struct __attribute__((packed)) hpet_table_t {
    struct sdt_hdr_t hdr;
    uint32_t event_timer_block_id;
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
    uint8_t hpet_number;
    uint16_t minimum_clock_tick;
    uint8_t page_protection;
};

struct __attribute__((packed)) hpet_timer_t {
    uint64_t config_and_capabilities;
    uint64_t comparator_value;
    uint64_t fsb_int_route;
    uint64_t reserved;
};

struct __attribute__((packed)) hpet_t {
    uint64_t general_capabilities;
    uint64_t reserved0;
    volatile uint64_t general_config;
    uint64_t reserved1;
    uint64_t general_int_status;
    uint64_t reserved2[25];
    uint64_t main_counter_val;
    uint64_t reserved3;
    struct hpet_timer_t timers[];
};

static struct hpet_t *hpet;
static uint64_t hpet_freq;

void hpet_init(void) {
    struct hpet_table_t *hpet_table = (struct hpet_table_t *) acpi_find_table("HPET");
    if (hpet_table == NULL) {
        klog_debug("HPET not supported");
        return;
    }

    vmm_map_hhdm(hpet_table->address);
    hpet = (struct hpet_t *) (hpet_table->address + vmm_get_hhdm_offset());

    uint64_t hpet_comparators_count = ((hpet->general_capabilities >> 8) & 0x1f) + 1;
    bool hpet_64_bit_compatible = hpet->general_capabilities & (1 << 13);
    uint64_t hpet_period = hpet->general_capabilities >> 32;

    hpet_freq = 1000000000000000 / hpet_period;
    klog_debug("HPET comparators count: %u", hpet_comparators_count);
    klog_debug("HPET 64-bit compatible: %s", hpet_64_bit_compatible ? "yes" : "no");
    klog_debug("HPET frequency: %llu Hz", hpet_freq);

    hpet->general_config = 0x0; // disable main counter
    hpet->main_counter_val = 0; // reset main counter
    hpet->general_config = 0x1; // enable main counter

    klog_info("HPET initialized");
}

static inline uint64_t ns_to_ticks(uint64_t ns) {
    return ns * hpet_freq / 1000000;
}

static inline uint64_t ticks_to_ns(uint64_t ticks) {
    return 1000000 * ticks / hpet_freq;
}

uint64_t hpet_get_ns(void) {
    return ticks_to_ns(hpet->main_counter_val);
}

void hpet_sleep_ns(uint64_t ns) {
    uint64_t hpet_counter_target = hpet->main_counter_val + ns_to_ticks(ns);
    while (hpet->main_counter_val < hpet_counter_target) {
        pause();
    }
}

bool hpet_is_supported(void) {
    return acpi_find_table("HPET") != NULL;
}
