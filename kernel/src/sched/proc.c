#include "sched/thread.h"
#include "sched/proc.h"

void proc_threads_init(struct proc_t *proc) {
    DLIST_INIT_SYNCED(proc->threads);
}

void proc_threads_add(struct proc_t *proc, struct thread_t *thread) {
    DLIST_INSERT_SYNCED(proc->threads, thread, proc_links);
}

void proc_threads_remove(struct proc_t *proc, struct thread_t *thread) {
    DLIST_DELETE_SYNCED(proc->threads, thread, proc_links);
}
