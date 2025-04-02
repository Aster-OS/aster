#include <stddef.h>

#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/list/dlist.h"
#include "lib/memutil.h"
#include "lib/spinlock/spinlock.h"
#include "lib/stacktrace/stacktrace.h"
#include "lib/strutil.h"
#include "memory/kmalloc/kmalloc.h"
#include "memory/vmm/vmm.h"
#include "mp/cpu.h"
#include "mp/mp.h"
#include "sched/proc.h"
#include "sched/sched.h"
#include "sched/thread.h"

static struct {
    struct spinlock_t lock;
    pid_t pid;
} pid_generator;

static struct {
    struct spinlock_t lock;
    tid_t tid;
} tid_generator;

static const size_t KTHREAD_STACK_SIZE = 32768;
static const uint64_t SCHED_TIMESLICE = 30000;
static uint8_t sched_vec;

static struct proc_t *proc_kernel;
DLIST_INSTANCE_ATOMIC(procs, struct proc_t, static);

static void *worker_free_dead_threads(void *arg);

extern void sched_thread_entry(void);
extern void sched_thread_switch(void **curr_sp, void **new_sp, bool prev_int_state);

static pid_t new_pid(void) {
    bool prev_int_state = cpu_set_int_state(false);
    spinlock_acquire(&pid_generator.lock);

    pid_t ret = pid_generator.pid;
    pid_generator.pid++;

    spinlock_release(&pid_generator.lock);
    cpu_set_int_state(prev_int_state);

    return ret;
}

static tid_t new_tid(void) {
    bool prev_int_state = cpu_set_int_state(false);
    spinlock_acquire(&tid_generator.lock);

    tid_t ret = tid_generator.tid;
    tid_generator.tid++;

    spinlock_release(&tid_generator.lock);
    cpu_set_int_state(prev_int_state);

    return ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static void proc_delete(struct proc_t *proc) {
    DLIST_DELETE_ATOMIC(procs, proc, prev, next);
}

#pragma GCC diagnostic pop

static void proc_insert(struct proc_t *proc) {
    DLIST_INSERT_ATOMIC(procs, proc, prev, next);
}

static void thread_queue_delete(struct thread_queue_t *queue, struct thread_t *thread, bool autolock) {
    if (autolock) {
        bool prev_int_state = cpu_set_int_state(false);
        DLIST_DELETE_ATOMIC(*queue, thread, prev, next);
        cpu_set_int_state(prev_int_state);
    } else {
        DLIST_DELETE(*queue, thread, prev, next);
    }
}

static void thread_queue_insert(struct thread_queue_t *queue, struct thread_t *thread, bool autolock) {
    if (autolock) {
        bool prev_int_state = cpu_set_int_state(false);
        DLIST_INSERT_ATOMIC(*queue, thread, prev, next);
        cpu_set_int_state(prev_int_state);
    } else {
        DLIST_INSERT(*queue, thread, prev, next);
    }
}

static void thread_queue_lock(struct thread_queue_t *queue) {
    bool prev_int_state = cpu_set_int_state(false);
    DLIST_LOCK(*queue);
    cpu_set_int_state(prev_int_state);
}

static void thread_queue_unlock(struct thread_queue_t *queue) {
    bool prev_int_state = cpu_set_int_state(false);
    DLIST_UNLOCK(*queue);
    cpu_set_int_state(prev_int_state);
}

static struct thread_t *search_ready_thread(struct thread_t *start) {
    struct thread_t *thread = start;
    while (thread) {
        if (thread->state == THREAD_STATE_READY) {
            return thread;
        }

        thread = thread->next;
    }

    return NULL;
}

static struct thread_t *get_next_thread(struct thread_queue_t *queue, struct thread_t *curr) {
    struct thread_t *next;

    thread_queue_lock(queue);

    // no thread running
    if (curr == NULL) {
        next = search_ready_thread(queue->head);
        goto ret;
    }

    // search for a thread after the current thread
    next = search_ready_thread(curr->next);
    if (next) {
        goto ret;
    }

    // search for a thread in the whole queue
    next = search_ready_thread(queue->head);

ret:
    thread_queue_unlock(queue);
    return next;
}

static struct cpu_t *pick_cpu(void) {
    static struct spinlock_t lock;
    static uint64_t cpu_round_robin;

    bool prev_int_state = cpu_set_int_state(false);
    spinlock_acquire(&lock);

    struct cpu_t *cpu = mp_get_cpus()[cpu_round_robin];
    cpu_round_robin = (cpu_round_robin + 1) % mp_get_cpu_count();

    spinlock_release(&lock);
    cpu_set_int_state(prev_int_state);

    return cpu;
}

static struct thread_t *create_thread(void *(*start)(void *), void *arg) {
    struct thread_t *thread = (struct thread_t *) kmalloc(sizeof(struct thread_t));

    void *kstack = kmalloc(KTHREAD_STACK_SIZE);
    uintptr_t kstack_bottom = (uintptr_t) kstack + KTHREAD_STACK_SIZE;
    uint64_t *sp = (uint64_t *) kstack_bottom;

    *(--sp) = (uint64_t) arg;
    *(--sp) = (uint64_t) start;
    *(--sp) = (uint64_t) sched_thread_entry;
    *(--sp) = 0; // rbx
    *(--sp) = 0; // rbp
    *(--sp) = 0; // r12
    *(--sp) = 0; // r13
    *(--sp) = 0; // r14
    *(--sp) = 0; // r15

    thread->kstack = kstack;
    thread->parent = proc_kernel;
    thread->state = THREAD_STATE_READY;
    thread->sp = sp;
    thread->tid = new_tid();

    proc_threads_add(thread->parent, thread);

    return thread;
}

struct proc_t *sched_new_proc(const char *name, phys_t pagemap) {
    if (!pagemap) {
        kpanic("Creating process page tables is not implemented");
    }

    struct proc_t *proc = kmalloc(sizeof(struct proc_t));
    size_t name_strlen = strlen(name);
    proc->name = kmalloc(name_strlen);
    memcpy(proc->name, name, name_strlen);
    proc->pagemap = pagemap;
    proc->pid = new_pid();
    DLIST_INIT_ATOMIC(proc->threads);

    klog_info("Created process \"%s\" with PID %llu", proc->name, proc->pid);

    proc_insert(proc);

    return proc;
}

struct thread_t *sched_new_kthread(void *(*start)(void *), void *arg) {
    struct thread_t *thread = create_thread(start, arg);
    struct cpu_t *picked_cpu = pick_cpu();
    thread_queue_insert(&picked_cpu->run_queue, thread, true);
    return thread;
}

struct thread_t *sched_new_thread(struct proc_t *parent, void *(*start)(void *), void *arg) {
    (void) parent, (void) start, (void) arg;
    kpanic("User threads are not implemented");
}

static void sched_int_handler(struct int_ctx_t *ctx) {
    (void) ctx;
    lapic_send_eoi();
    sched_yield();
}

void sched_init(void) {
    sched_vec = interrupts_alloc_vector();
    interrupts_set_handler(sched_vec, sched_int_handler);

    DLIST_INIT_ATOMIC(procs);
    proc_kernel = sched_new_proc("kernel", vmm_get_kernel_pagemap());

    klog_info("Scheduler initialized");
}

void sched_init_cpu(void) {
    struct cpu_t *cpu = get_cpu();
    cpu->curr_thread = NULL;
    DLIST_INIT_ATOMIC(cpu->dead_queue);
    DLIST_INIT_ATOMIC(cpu->run_queue);

    struct thread_t *worker = create_thread(worker_free_dead_threads, NULL);
    thread_queue_insert(&cpu->run_queue, worker, true);
}

void sched_thread_exit(void *thread_returned) {
    (void) thread_returned;

    struct cpu_t *cpu = get_cpu();
    struct thread_t *curr = cpu->curr_thread;
    curr->state = THREAD_STATE_DEAD;

    proc_threads_remove(curr->parent, curr);
    thread_queue_delete(&cpu->run_queue, curr, true);
    thread_queue_insert(&cpu->dead_queue, curr, true);

    sched_yield();

    kpanic("Dead thread scheduled");
}

void sched_yield(void) {
    bool prev_int_state = cpu_set_int_state(false);
    lapic_timer_stop();

    struct cpu_t *cpu = get_cpu();
    struct thread_t *curr = cpu->curr_thread;
    struct thread_t *next;

    void **curr_sp = NULL;
    void **new_sp;

    if (curr) {
        curr_sp = &curr->sp;
        // if the current thread is NOT DEAD, we mark it as READY,
        // so that get_next_thread() can find and return it
        // if there are no other threads to run
        if (curr->state != THREAD_STATE_DEAD) { 
            curr->state = THREAD_STATE_READY;
        }
    }

    next = get_next_thread(&cpu->run_queue, curr);
    if (next == NULL) {
        kpanic("No thread to run");
    }

    new_sp = &next->sp;
    next->state = THREAD_STATE_RUNNING;

    cpu->curr_thread = next;

    lapic_timer_one_shot(SCHED_TIMESLICE, sched_vec);
    sched_thread_switch(curr_sp, new_sp, prev_int_state);
}

static void *worker_free_dead_threads(void *arg) {
    (void) arg;

    while (1) {
        bool prev_int_state = cpu_set_int_state(false);
        struct cpu_t *cpu = get_cpu();
        thread_queue_lock(&cpu->dead_queue);

        if (cpu->dead_queue.head == NULL) {
            goto yield;
        }

        struct thread_t *thread = cpu->dead_queue.head;
        while (thread) {
            struct thread_t *next = thread->next; // avoid use after free

            thread_queue_delete(&cpu->dead_queue, thread, false);
            kfree(thread->kstack);
            kfree(thread);

            thread = next;
        }

yield:
        thread_queue_unlock(&cpu->dead_queue);
        cpu_set_int_state(prev_int_state);
        sched_yield();
    }

    return NULL;
}
