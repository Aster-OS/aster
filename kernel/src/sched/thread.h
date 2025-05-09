#pragma once

#include <stdint.h>

#include "sched/proc.h"

typedef uint16_t tid_t;

enum thread_state_t {
    THREAD_STATE_DEAD,
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING
};

struct thread_t {
    struct thread_t *prev;
    struct thread_t *next;
    void *kstack;
    struct proc_t *parent;
    struct thread_t *proc_prev;
    struct thread_t *proc_next;
    void *sp;
    enum thread_state_t state;
    tid_t tid;
};
