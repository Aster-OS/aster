#pragma once

#include <stdint.h>

#include "lib/list/dlist.h"
#include "memory/vmm/vmm.h"

typedef uint16_t pid_t;

struct thread_t;

struct proc_t {
    struct {
        struct proc_t *prev;
        struct proc_t *next;
    } links;
    struct pagemap_t *pagemap;
    char *name;
    pid_t pid;
    DLIST_HEAD_SYNCED(threads, struct thread_t);
};

void proc_threads_init(struct proc_t *proc);
void proc_threads_add(struct proc_t *proc, struct thread_t *thread);
void proc_threads_remove(struct proc_t *proc, struct thread_t *thread);
