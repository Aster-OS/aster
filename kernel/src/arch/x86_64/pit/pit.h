#pragma once

#include "arch/x86_64/interrupts/interrupts.h"

uint64_t pit_get_ns(void);
void pit_init(void);
void pit_sleep_ns(uint64_t ns);
