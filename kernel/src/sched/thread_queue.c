#include <stddef.h>

#include "lib/spinlock/spinlock.h"
#include "mp/cpu.h"
#include "sched/thread_queue.h"

void thread_queue_delete(struct thread_queue_t *queue, struct thread_t *thread, bool autolock) {
    bool prev_int_state = false; // avoid maybe-uninitialized warning
    if (autolock) {
        prev_int_state = cpu_set_int_state(false);
        thread_queue_lock(queue);
    }

    if (thread->prev) {
        thread->prev->next = thread->next;
    }

    if (thread->next) {
        thread->next->prev = thread->prev;
    }

    if (thread == queue->head) {
        queue->head = queue->head->next;
    }

    if (autolock) {
        thread_queue_unlock(queue);
        cpu_set_int_state(prev_int_state);
    }
}

void thread_queue_insert(struct thread_queue_t *queue, struct thread_t *thread, bool autolock) {
    bool prev_int_state;
    if (autolock) {
        prev_int_state = cpu_set_int_state(false);
        thread_queue_lock(queue);
    }

    if (queue->head == NULL) {
        queue->head = thread;
        queue->head->prev = NULL;
        queue->head->next = NULL;
        goto ret;
    }

    thread->prev = NULL;
    thread->next = queue->head;
    queue->head->prev = thread;
    queue->head = thread;

ret:
    if (autolock) {
        thread_queue_unlock(queue);
        cpu_set_int_state(prev_int_state);
    }
}

void thread_queue_lock(struct thread_queue_t *queue) {
    spinlock_acquire(&queue->lock);
}

void thread_queue_unlock(struct thread_queue_t *queue) {
    spinlock_release(&queue->lock);
}
