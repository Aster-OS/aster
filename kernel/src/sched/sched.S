.section .text

# bool cpu_set_int_state(bool enabled);
.extern cpu_set_int_state

# void sched_thread_exit(void *thread_returned);
.extern sched_thread_exit

# void sched_thread_entry(void);
.global sched_thread_entry
sched_thread_entry:
    pop %rax # start
    pop %rdi # arg

    # align stack on a 16-byte boundary
    mov $0xf, %rcx
    not %rcx
    and %rcx, %rsp

    # call the thread start routine
    call *%rax

    # pass the value returned by the thread
    # to the exit routine
    mov %rax, %rdi
    call sched_thread_exit

    # unreachable
    ud2

# void sched_thread_switch(void **curr_sp, void **new_sp, bool prev_int_state);
.global sched_thread_switch
sched_thread_switch:
    # skip saving state if current sp is NULL
    test %rdi, %rdi
    jz skip_thread_save

    # save call-preserved registers; save current sp
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    mov %rsp, (%rdi)

skip_thread_save:
    # restore new sp; restore call-preserved registers
    mov (%rsi), %rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx

    # restore previous interrupt state
    mov %rdx, %rdi
    call cpu_set_int_state

    ret
