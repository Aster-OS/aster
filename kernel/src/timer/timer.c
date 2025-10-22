#include "acpi/hpet.h"
#include "arch/x86_64/pit/pit.h"
#include "klog/klog.h"
#include "timer/timer.h"

struct timer_t {
    uint64_t (*get_ns)(void);
    void (*sleep_ns)(uint64_t ns);
};

static struct timer_t system_timer;

void timer_init(void) {
    if (hpet_is_supported()) {
        hpet_init();
        system_timer.get_ns = hpet_get_ns;
        system_timer.sleep_ns = hpet_sleep_ns;
        klog_info("Using HPET as system timer");
    } else {
        pit_init();
        system_timer.get_ns = pit_get_ns;
        system_timer.sleep_ns = pit_sleep_ns;
        klog_info("Using PIT as system timer");
    }
}

uint64_t timer_get_ns(void) {
    return system_timer.get_ns();
}

void timer_sleep_ns(uint64_t ns) {
    system_timer.sleep_ns(ns);
}
