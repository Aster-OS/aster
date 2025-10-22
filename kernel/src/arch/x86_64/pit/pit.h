#pragma once

#include <stdint.h>

uint64_t pit_get_ns(void);
void pit_init(void);
void pit_sleep_ns(uint64_t ns);
