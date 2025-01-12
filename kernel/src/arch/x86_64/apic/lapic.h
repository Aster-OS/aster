#pragma once

#include <stdint.h>

void lapic_init(void);
void lapic_send_eoi(void);
void lapic_timer_calibrate(uint64_t calibration_sleep_ns);
void lapic_timer_one_shot(uint64_t ns, uint8_t vec);
void lapic_timer_periodic(uint64_t ns, uint8_t vec);
void lapic_timer_stop(void);
