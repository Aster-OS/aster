#include <stddef.h>

#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/elf/symbols.h"
#include "lib/list/dlist.h"
#include "lib/memutil.h"
#include "lib/spinlock/spinlock.h"
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
static DLIST_HEAD_SYNCED(procs, struct proc_t);

static void *worker_free_dead_threads(void *arg);

extern void sched_thread_entry(void);
extern void sched_thread_switch(void **curr_sp_ptr, void **next_sp_ptr);

static pid_t new_pid(void) {
    spin_lock_irqsave(&pid_generator.lock);

    pid_t ret = pid_generator.pid;
    pid_generator.pid++;

    spin_unlock_irqrestore(&pid_generator.lock);

    return ret;
}

static tid_t new_tid(void) {
    spin_lock_irqsave(&pid_generator.lock);

    tid_t ret = tid_generator.tid;
    tid_generator.tid++;

    spin_unlock_irqrestore(&pid_generator.lock);

    return ret;
}

static struct thread_t *search_ready_thread(struct thread_t *start) {
    struct thread_t *thread = start;
    while (thread) {
        if (thread->state == THREAD_STATE_READY) {
            return thread;
        }

        thread = thread->links.next;
    }

    return NULL;
}

static struct thread_t *get_next_thread(struct thread_queue_t *queue, struct thread_t *curr) {
    struct thread_t *next;

    DLIST_LOCK_IRQSAVE(*queue);

    // search for a thread after the current thread
    next = search_ready_thread(curr->links.next);
    if (next) {
        goto ret;
    }

    // search for a thread in the whole queue
    next = search_ready_thread(queue->head);

ret:
    DLIST_UNLOCK_IRQRESTORE(*queue);
    return next;
}

static struct cpu_t *pick_cpu(void) {
    static struct spinlock_t lock = SPINLOCK_STATIC_INIT;
    static uint64_t rr_next_cpu;

    spin_lock_irqsave(&lock);

    struct cpu_t *cpu = mp_get_cpus()[rr_next_cpu];
    rr_next_cpu = (rr_next_cpu + 1) % mp_get_cpu_count();

    spin_unlock_irqrestore(&lock);

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
    DLIST_INIT_SYNCED(proc->threads);

    klog_info("Created process \"%s\" with PID %llu", proc->name, proc->pid);

    DLIST_INSERT_SYNCED(procs, proc, links);

    return proc;
}

struct thread_t *sched_new_kthread(void *(*start)(void *), void *arg) {
    struct thread_t *thread = create_thread(start, arg);
    struct cpu_t *picked_cpu = pick_cpu();
    DLIST_INSERT_SYNCED(picked_cpu->run_queue, thread, links);
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

    DLIST_INIT_SYNCED(procs);
    proc_kernel = sched_new_proc("kernel", vmm_get_kernel_pagemap());

    klog_info("Scheduler initialized");
}

void sched_init_cpu(void) {
    struct cpu_t *cpu = get_cpu();
    cpu->curr_thread = NULL;
    DLIST_INIT_SYNCED(cpu->dead_queue);
    DLIST_INIT_SYNCED(cpu->run_queue);

    struct thread_t *dummy = create_thread(NULL, NULL);
    dummy->state = THREAD_STATE_DEAD;
    cpu->curr_thread = dummy;
    DLIST_INSERT_SYNCED(cpu->run_queue, dummy, links);

    // TODO a reaper worker thread for all CPUs?
    struct thread_t *worker = create_thread(worker_free_dead_threads, NULL);
    DLIST_INSERT_SYNCED(cpu->run_queue, worker, links);
}

void sched_thread_exit(void *thread_returned) {
    (void) thread_returned;

    struct cpu_t *cpu = get_cpu();
    struct thread_t *curr = cpu->curr_thread;
    curr->state = THREAD_STATE_DEAD;

    proc_threads_remove(curr->parent, curr);
    DLIST_DELETE_SYNCED(cpu->run_queue, curr, links);
    DLIST_INSERT_SYNCED(cpu->dead_queue, curr, links);

    klog_info("Thread %d finished executing on CPU %d", curr->tid, cpu->id);

    sched_yield();

    kpanic("Dead thread scheduled");
}

void sched_yield(void) {
    bool old_int_state = interrupts_set(false);

    lapic_timer_stop();

    struct cpu_t *cpu = get_cpu();
    struct thread_t *curr = cpu->curr_thread;
    struct thread_t *next;

    void **curr_sp_ptr = &curr->sp;
    void **next_sp_ptr;

    if (curr->state != THREAD_STATE_DEAD) { 
        curr->state = THREAD_STATE_READY;
    }

    next = get_next_thread(&cpu->run_queue, curr);
    if (next == NULL) {
        kpanic("No thread to run");
    }
    next_sp_ptr = &next->sp;
    next->state = THREAD_STATE_RUNNING;

    cpu->curr_thread = next;

    lapic_timer_one_shot(SCHED_TIMESLICE, sched_vec);
    sched_thread_switch(curr_sp_ptr, next_sp_ptr);

    interrupts_set(old_int_state);
}

static void *worker_free_dead_threads(void *arg) {
    (void) arg;

    while (1) {
        bool old_int_state = interrupts_set(false);
        struct cpu_t *cpu = get_cpu();
        spin_lock(&cpu->dead_queue.lock);

        if (cpu->dead_queue.head == NULL) {
            goto yield;
        }

        struct thread_t *thread = cpu->dead_queue.head;
        while (thread) {
            // store the next thread's address before freeing memory
            struct thread_t *next = thread->links.next;

            // dead_queue is already locked
            DLIST_DELETE(cpu->dead_queue, thread, links);
            kfree(thread->kstack);
            kfree(thread);

            thread = next;
        }

yield:
        spin_unlock(&cpu->dead_queue.lock);
        interrupts_set(old_int_state);
        sched_yield();
    }

    return NULL;
}
