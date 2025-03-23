#pragma once

#include <stdbool.h>

#include "lib/spinlock/spinlock_t.h"
#include "sched/thread.h"

struct thread_queue_t {
    struct thread_t *head;
    struct spinlock_t lock;
};

void thread_queue_delete(struct thread_queue_t *queue, struct thread_t *thread, bool autolock);
void thread_queue_insert(struct thread_queue_t *queue, struct thread_t *thread, bool autolock);
void thread_queue_lock(struct thread_queue_t *queue);
void thread_queue_unlock(struct thread_queue_t *queue);
