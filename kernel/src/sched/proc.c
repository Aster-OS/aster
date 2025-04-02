#include "sched/thread.h"
#include "sched/proc.h"

void proc_threads_init(struct proc_t *proc) {
    DLIST_INIT_ATOMIC(proc->threads);
}

void proc_threads_add(struct proc_t *proc, struct thread_t *thread) {
    DLIST_INSERT_ATOMIC(proc->threads, thread, proc_prev, proc_next);
}

void proc_threads_remove(struct proc_t *proc, struct thread_t *thread) {
    DLIST_DELETE_ATOMIC(proc->threads, thread, proc_prev, proc_next);
}
