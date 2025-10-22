.section .text

.extern common_int_handler 

isr_common_entry:
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
    call common_int_handler

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

    add $16, %rsp # pop error code and interrupt vector

    iretq

.macro isr_ec vector
    isr_\vector:
        push $\vector
        jmp isr_common_entry
.endm

.macro isr_no_ec vector
    isr_\vector:
        push $0
        push $\vector
        jmp isr_common_entry
.endm

# CPU Exceptions (0x00 - 0x1f)
isr_no_ec 0
isr_no_ec 1
isr_no_ec 2
isr_no_ec 3
isr_no_ec 4
isr_no_ec 5
isr_no_ec 6
isr_no_ec 7
isr_ec 8
isr_no_ec 9
isr_ec 10
isr_ec 11
isr_ec 12
isr_ec 13
isr_ec 14
isr_no_ec 15
isr_no_ec 16
isr_ec 17
isr_no_ec 18
isr_no_ec 19
isr_no_ec 20
isr_ec 21
isr_no_ec 22
isr_no_ec 23
isr_no_ec 24
isr_no_ec 25
isr_no_ec 26
isr_no_ec 27
isr_no_ec 28
isr_ec 29
isr_ec 30
isr_no_ec 31

.altmacro

# 0x20 - 0xff
.set i, 32
.rept 224
    isr_no_ec %i
.set i, i + 1
.endr

.macro write_isr_addr vector
    .quad isr_\vector
.endm

.section .rodata
.global isr_array
isr_array:
.set i, 0
.rept 256
    write_isr_addr %i
.set i, i + 1
.endr
