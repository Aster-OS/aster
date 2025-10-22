#pragma once

#include <stdint.h>

void lapic_init(void);
void lapic_init_cpu(void);
void lapic_ipi(uint8_t vec, uint32_t dest_lapic_id);
void lapic_ipi_all(uint8_t vec);
void lapic_ipi_all_no_self(uint8_t vec);
void lapic_ipi_self(uint8_t vec);
void lapic_send_eoi(void);
void lapic_timer_calibrate(void);
void lapic_timer_one_shot(uint64_t ns, uint8_t vec);
void lapic_timer_periodic(uint64_t ns, uint8_t vec);
void lapic_timer_stop(void);
