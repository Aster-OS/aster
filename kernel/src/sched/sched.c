#include "sched/sched.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/asm.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/list/dlist.h"
#include "lib/memutil.h"
#include "lib/spinlock/spinlock.h"
#include "lib/strutil.h"
#include "memory/kmalloc/kmalloc.h"
#include "memory/vmm/vmm.h"
#include "mp/cpu.h"
#include "mp/mp.h"
#include "sched/proc.h"
#include "sched/thread.h"

void sched_kthread_exit(void *thread_returned);

extern void sched_kthread_entry(void);
extern void sched_thread_entry(void);
extern void sched_thread_switch(void **curr_sp_ptr, void **next_sp_ptr);

static struct {
    struct spinlock_t lock;
    pid_t pid;
} pid_generator;

static struct {
    struct spinlock_t lock;
    tid_t tid;
} tid_generator;

static uint64_t const SCHED_TIMESLICE = 30000;

static uint8_t sched_int_vec;

static struct proc_t *kernel_proc;
static DLIST_HEAD_SYNCED(procs, struct proc_t);

static void *worker_free_dead_threads(void *arg);

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

static struct thread_t *get_next_thread(struct thread_queue_t *queue,
                                        struct thread_t *curr) {
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
    static uint64_t next_cpu;

    spin_lock_irqsave(&lock);

    struct cpu_t *cpu = mp_get_cpus()[next_cpu];
    next_cpu = (next_cpu + 1) % mp_get_cpu_count();

    spin_unlock_irqrestore(&lock);

    return cpu;
}

static struct thread_t *new_thread(struct proc_t *proc, thread_start_t start,
                                   void *arg, bool is_user) {
    struct thread_t *thread =
        (struct thread_t *) kmalloc(sizeof(struct thread_t));

    uint64_t *sp = (uint64_t *) (thread->kstack + sizeof(thread->kstack));
    if (is_user) {
        thread->ustack = kmalloc(8 << 20);
        *(--sp) = (uint64_t) thread->ustack;
        // TODO: arg unused
        *(--sp) = (uint64_t) start;
        *(--sp) = (uint64_t) sched_thread_entry;
    } else {
        thread->ustack = NULL;
        *(--sp) = (uint64_t) arg;
        *(--sp) = (uint64_t) start;
        *(--sp) = (uint64_t) sched_kthread_entry;
    }
    *(--sp) = 0; // rbx
    *(--sp) = 0; // rbp
    *(--sp) = 0; // r12
    *(--sp) = 0; // r13
    *(--sp) = 0; // r14
    *(--sp) = 0; // r15

    thread->is_user = is_user;
    thread->kstack_sp = sp;
    thread->proc = proc;
    thread->state = THREAD_STATE_READY;
    thread->tid = new_tid();

    proc_threads_add(proc, thread);

    return thread;
}

void sched_exit(void) {
    struct cpu_t *cpu = get_cpu();
    struct proc_t *proc = cpu->curr_thread->proc;
    DLIST_LOCK_IRQSAVE(proc->threads);
    for (struct thread_t *thread = proc->threads.head; thread != NULL;
         thread = thread->proc_links.next) {
        thread->state = THREAD_STATE_DEAD;
        DLIST_DELETE_SYNCED(cpu->run_queue, thread, links);
        DLIST_INSERT_SYNCED(cpu->dead_queue, thread, links);
    }

    sched_yield();

    // FIX: Free memory
}

struct thread_t *sched_new_kthread(thread_start_t start, void *arg) {
    struct thread_t *thread = new_thread(kernel_proc, start, arg, false);
    struct cpu_t *cpu = pick_cpu();
    DLIST_INSERT_SYNCED(cpu->run_queue, thread, links);
    return thread;
}

struct thread_t *sched_new_thread(struct proc_t *proc, thread_start_t start,
                                  void *arg) {
    struct thread_t *thread = new_thread(proc, start, arg, true);
    struct cpu_t *cpu = pick_cpu();
    DLIST_INSERT_SYNCED(cpu->run_queue, thread, links);
    return thread;
}

struct proc_t *sched_new_proc(char const *name, struct pagemap_t *pagemap) {
    struct proc_t *proc = kmalloc(sizeof(struct proc_t));

    size_t len = kstrnlen(name, 32);
    proc->name = kmalloc(len);
    kmemcpy(proc->name, name, len);

    proc->pagemap = pagemap;
    proc->pid = new_pid();
    DLIST_INIT_SYNCED(proc->threads);

    klog_info("Created process \"%s\" with PID %llu", proc->name, proc->pid);

    DLIST_INSERT_SYNCED(procs, proc, links);

    return proc;
}

static void sched_int_handler(struct int_ctx_t *ctx) {
    (void) ctx;
    lapic_eoi();
    sched_yield();
}

void sched_init(void) {
    sched_int_vec = interrupts_alloc_vector();
    interrupts_set_handler(sched_int_vec, sched_int_handler, false);

    DLIST_INIT_SYNCED(procs);
    kernel_proc = sched_new_proc("kernel", vmm_kernel_pagemap());

    klog_info("Scheduler initialized");
}

void sched_init_cpu(void) {
    struct cpu_t *cpu = get_cpu();
    cpu->curr_thread = NULL;
    DLIST_INIT_SYNCED(cpu->dead_queue);
    DLIST_INIT_SYNCED(cpu->run_queue);

    // TODO:
    // if worker is made the curr_thread, calling sched_yield from kernel_entry
    // makes the kernel think that is the worker thread and saves the
    // kernel_entry state when context switching to worker, the scheduler
    // unknowningly restores the kernel_entry state

    // TODO: ?
    // make the scheduler think that the kernel entry point was a thread that is
    // now dead
    struct thread_t *dead_thread = new_thread(kernel_proc, NULL, NULL, false);
    dead_thread->state = THREAD_STATE_DEAD;
    cpu->curr_thread = dead_thread;
    DLIST_INSERT_SYNCED(cpu->run_queue, dead_thread, links);

    struct thread_t *worker =
        new_thread(kernel_proc, worker_free_dead_threads, NULL, false);
    DLIST_INSERT_SYNCED(cpu->run_queue, worker, links);
}

void sched_kthread_exit(void *thread_returned) {
    (void) thread_returned;

    struct cpu_t *cpu = get_cpu();
    struct thread_t *curr = cpu->curr_thread;
    curr->state = THREAD_STATE_DEAD;

    proc_threads_remove(curr->proc, curr);
    DLIST_DELETE_SYNCED(cpu->run_queue, curr, links);
    DLIST_INSERT_SYNCED(cpu->dead_queue, curr, links);

    klog_info("Kernel thread exit tid=%u, cpu=%u", curr->tid, cpu->id);

    sched_yield();

    kpanic("Dead thread scheduled");
}

void sched_yield(void) {
    bool old_int_state = interrupts_set(false);

    lapic_timer_stop();

    struct cpu_t *cpu = get_cpu();
    struct thread_t *curr = cpu->curr_thread;
    struct thread_t *next;

    void **curr_sp_ptr = &curr->kstack_sp;
    void **next_sp_ptr;

    if (curr->state != THREAD_STATE_DEAD) {
        curr->state = THREAD_STATE_READY;
    }

    next = get_next_thread(&cpu->run_queue, curr);
    if (next == NULL) {
        kpanic("No thread to run");
    }
    next_sp_ptr = &next->kstack_sp;
    next->state = THREAD_STATE_RUNNING;

    cpu->curr_thread = next;

    lapic_timer_one_shot(SCHED_TIMESLICE, sched_int_vec);

    vmm_load_pagemap(next->proc->pagemap);
    cpu->tss.rsp[0] = (uint64_t) (next->kstack + sizeof(next->kstack));
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

            if (thread->is_user) {
                kfree(thread->ustack);
            }
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
