#pragma once

#include <stdint.h>

uint64_t timer_get_ns(void);
void timer_init(void);
void timer_sleep_ns(uint64_t ns);
