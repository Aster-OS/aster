#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "limine.h"
#include "mp/cpu.h"

struct cpu_t *mp_get_bsp(void);
uint64_t mp_get_cpu_count(void);
struct cpu_t **mp_get_cpus(void);
__attribute__((noreturn))
void mp_halt_all_cpus(void);
void mp_init(struct limine_mp_response *mp);
void mp_init_bsp(struct limine_mp_response *mp);
