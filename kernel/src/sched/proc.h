#pragma once

#include <stdint.h>

#include "lib/list/dlist.h"
#include "memory/pmm/pmm.h"

struct thread_t;

typedef uint16_t pid_t;

struct proc_t {
    struct {
        struct proc_t *prev;
        struct proc_t *next;
    } links;
    char *name;
    phys_t pagemap;
    pid_t pid;
    DLIST_HEAD_SYNCED(threads, struct thread_t);
};

void proc_threads_init(struct proc_t *proc);
void proc_threads_add(struct proc_t *proc, struct thread_t *thread);
void proc_threads_remove(struct proc_t *proc, struct thread_t *thread);
