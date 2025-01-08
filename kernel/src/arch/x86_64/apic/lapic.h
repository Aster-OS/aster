#pragma once

#include <stdint.h>

void lapic_init(void);
void lapic_send_eoi(void);
