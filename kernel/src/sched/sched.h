#pragma once

#include "memory/vmm/vmm.h"
#include "sched/proc.h"
#include "sched/thread.h"

typedef void *(*thread_start_t)(void *);

void sched_exit(void);
void sched_init(void);
void sched_init_cpu(void);
struct proc_t *sched_new_proc(char const *name, struct pagemap_t *pagemap);
struct thread_t *sched_new_kthread(thread_start_t start, void *arg);
struct thread_t *sched_new_thread(struct proc_t *proc, thread_start_t start,
                                  void *arg);
void sched_yield(void);
