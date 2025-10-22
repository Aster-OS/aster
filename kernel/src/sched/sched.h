#pragma once

#include "memory/pmm/pmm.h"
#include "sched/proc.h"
#include "sched/thread.h"

void sched_init(void);
void sched_init_cpu(void);
struct proc_t *sched_new_proc(const char *name, phys_t pagemap);
struct thread_t *sched_new_kthread(void *(*start)(void *), void *arg);
struct thread_t *sched_new_thread(struct proc_t *proc, void *(*start)(void *), void *arg);
void sched_yield(void);
