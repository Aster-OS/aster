.section .text

.global isr_table
.extern isr_handler 

isr_frame_assembler:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rdi
    push %rsi
    push %rbp

    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15

    mov %cr0, %rax
    push %rax
    mov %cr2, %rax
    push %rax
    mov %cr3, %rax
    push %rax
    mov %cr4, %rax
    push %rax

    mov %rsp, %rdi
    call isr_handler

    pop %rax
    mov %rax, %cr4
    pop %rax
    mov %rax, %cr3
    pop %rax
    mov %rax, %cr2
    pop %rax
    mov %rax, %cr0

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8

    pop %rbp
    pop %rsi
    pop %rdi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax

    add $16, %rsp # remove EC (error code) and vector number from the stack

    iretq

.macro create_isr_with_ec vector
    isr_vector_\vector:
        push $\vector
        jmp isr_frame_assembler
.endm

.macro create_isr_without_ec vector
    isr_vector_\vector:
        push $0
        push $\vector
        jmp isr_frame_assembler
.endm

create_isr_without_ec 0
create_isr_without_ec 1
create_isr_without_ec 2
create_isr_without_ec 3
create_isr_without_ec 4
create_isr_without_ec 5
create_isr_without_ec 6
create_isr_without_ec 7

create_isr_with_ec 8
create_isr_without_ec 9
create_isr_with_ec 10
create_isr_with_ec 11
create_isr_with_ec 12
create_isr_with_ec 13
create_isr_with_ec 14
create_isr_without_ec 15

create_isr_without_ec 16
create_isr_with_ec 17
create_isr_without_ec 18
create_isr_without_ec 19
create_isr_without_ec 20
create_isr_with_ec 21
create_isr_without_ec 22
create_isr_without_ec 23

create_isr_without_ec 24
create_isr_without_ec 25
create_isr_without_ec 26
create_isr_without_ec 27
create_isr_without_ec 28
create_isr_with_ec 29
create_isr_with_ec 30
create_isr_without_ec 31

.altmacro

.set i, 32
.rept 224
    create_isr_without_ec %i
.set i, i + 1
.endr

.macro write_isr_label vector
    .quad isr_vector_\vector
.endm

.section .rodata
isr_table:
.set i, 0
.rept 256
    write_isr_label %i
.set i, i + 1
.endr
