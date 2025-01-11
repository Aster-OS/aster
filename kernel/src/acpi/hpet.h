#pragma once

#include <stdbool.h>
#include <stdint.h>

uint64_t hpet_get_ns(void);
void hpet_init(void);
void hpet_sleep_ns(uint64_t ns);
bool hpet_is_supported(void);
