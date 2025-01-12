#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "limine.h"
#include "mp/cpu.h"

struct cpu_t *mp_get_bsp(void);
void mp_init_aps(struct limine_mp_response *mp);
void mp_init_bsp(struct limine_mp_response *mp);
