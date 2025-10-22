.section .text

# void sched_thread_exit(void *thread_returned);
.extern sched_thread_exit

# void sched_thread_entry(void);
.global sched_thread_entry
sched_thread_entry:
    pop %rax # start
    pop %rdi # arg

    # align stack on a 16-byte boundary
    and $0xfffffffffffffff0, %rsp

    # call the thread start routine
    call *%rax

    # pass the value returned by the thread
    # to the exit routine
    mov %rax, %rdi
    call sched_thread_exit

    # unreachable
    ud2

# void sched_thread_switch(void **curr_sp_ptr, void **next_sp_ptr);
.global sched_thread_switch
sched_thread_switch:
    # save call-preserved registers; save current sp
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    mov %rsp, (%rdi)

    # restore new sp; restore call-preserved registers
    mov (%rsi), %rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx

    ret
