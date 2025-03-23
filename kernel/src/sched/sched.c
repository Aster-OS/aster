#include <stddef.h>

#include "arch/x86_64/apic/lapic.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/spinlock/spinlock.h"
#include "lib/stacktrace/stacktrace.h"
#include "memory/kmalloc/kmalloc.h"
#include "mp/cpu.h"
#include "mp/mp.h"
#include "sched/sched.h"
#include "sched/thread.h"

static struct {
    tid_t val;
    struct spinlock_t lock;
} last_tid;

static const size_t KTHREAD_STACK_SIZE = 32768;
static const uint64_t SCHED_TIMESLICE = 30000;
static uint8_t sched_vec;

static void *worker_free_dead_threads(void *arg);

extern void sched_thread_entry(void);
extern void sched_thread_switch(void **curr_sp, void **new_sp, bool prev_int_state);

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

struct thread_t *sched_new_kthread(void *(*start)(void *), void *arg, struct cpu_t *cpu) {
    struct thread_t *thread = (struct thread_t *) kmalloc(sizeof(struct thread_t));

    thread->state = THREAD_STATE_READY;

    bool prev_int_state = cpu_set_int_state(false);
    spinlock_acquire(&last_tid.lock);

    thread->tid = last_tid.val;
    last_tid.val++;

    spinlock_release(&last_tid.lock);
    cpu_set_int_state(prev_int_state);

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
    thread->sp = sp;

    struct cpu_t *picked_cpu;
    if (cpu) {
        picked_cpu = cpu;
    } else {
        picked_cpu = pick_cpu();
    }

    thread_queue_insert(&picked_cpu->run_queue, thread, true);

    klog_info("Thread %llu scheduled on CPU %llu", thread->tid, picked_cpu->id);

    return thread;
}

static void sched_int_handler(struct int_ctx_t *ctx) {
    (void) ctx;
    lapic_send_eoi();
    sched_yield();
}

void sched_init(void) {
    sched_vec = interrupts_alloc_vector();
    interrupts_set_handler(sched_vec, sched_int_handler);
    klog_info("Scheduler initialized");
}

void sched_init_cpu(void) {
    struct cpu_t *cpu = get_cpu();
    cpu->curr_thread = NULL;
    cpu->dead_queue.head = NULL;
    cpu->dead_queue.lock = SPINLOCK_INIT;
    cpu->run_queue.head = NULL;
    cpu->run_queue.lock = SPINLOCK_INIT;

    sched_new_kthread(worker_free_dead_threads, NULL, cpu);
}

void sched_thread_exit(void *thread_returned) {
    (void) thread_returned;

    struct cpu_t *cpu = get_cpu();
    struct thread_t *curr = cpu->curr_thread;
    curr->state = THREAD_STATE_DEAD;

    thread_queue_delete(&cpu->run_queue, curr, true);
    thread_queue_insert(&cpu->dead_queue, curr, true);

    klog_info("Thread %llu exited", curr->tid);

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
