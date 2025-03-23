#pragma once

#include <stdint.h>

#include "memory/pmm/pmm.h"
#include "mp/cpu.h"
#include "sched/thread.h"

void sched_init(void);
void sched_init_cpu(void);
struct thread_t *sched_new_kthread(void *(*start)(void *), void *arg, struct cpu_t *cpu);
void sched_yield(void);
