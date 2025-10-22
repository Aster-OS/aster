#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "limine.h"
#include "mp/cpu.h"

struct cpu_t *mp_get_bsp(void);
uint64_t mp_get_cpu_count(void);
struct cpu_t **mp_get_cpus(void);
uint8_t mp_get_halt_vector(void);
void mp_init(struct limine_mp_response *mp);
void mp_init_early(struct limine_mp_response *mp);
bool mp_x2apic_enabled(void);
