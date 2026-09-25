#include "arch/x86_64/gdt/gdt-sel.h"

.section .text

// void sched_kthread_exit(void *thread_returned);
.extern sched_kthread_exit

// void sched_kthread_entry(void);
.global sched_kthread_entry
sched_kthread_entry:
    pop %rax // thread start
    pop %rdi // thread arg

    // align stack on a 16-byte boundary
    and $0xfffffffffffffff0, %rsp

    // call thread start routine
    call *%rax

    // pass the value returned by the thread
    // to the exit routine
    mov %rax, %rdi
    call sched_kthread_exit

    // unreachable
    ud2

// void sched_thread_entry(void);
.global sched_thread_entry
sched_thread_entry:
    pop %rdi # thread start
    pop %rsi # ustack

    push $GDT_SEL_UDATA
    push %rsi
    pushf
    push $GDT_SEL_UCODE
    push %rdi

    xor %rax, %rax
    xor %rbx, %rbx
    xor %rcx, %rcx
    xor %rdx, %rdx
    xor %rdi, %rsi
    xor %rsi, %rsi
    xor %rbp, %rbp
    xor %r8, %r8
    xor %r9, %r9
    xor %r10, %r10
    xor %r11, %r11
    xor %r12, %r12
    xor %r13, %r13
    xor %r14, %r14
    xor %r15, %r15

    iretq

// void sched_thread_switch(void **curr_sp_ptr, void **next_sp_ptr);
.global sched_thread_switch
sched_thread_switch:
    // save call-preserved registers; save current sp
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    mov %rsp, (%rdi)

    // restore next sp; restore call-preserved registers
    mov (%rsi), %rsp
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx

    ret
