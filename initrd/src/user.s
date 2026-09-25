.section .text

.global _start
_start:
    mov $0x1, %rdi
    mov $msg, %rsi
    mov $msg_len, %rdx

    mov $0x5, %rcx
loop:
    mov $0x1, %rax
    int $0xf1

    sub $0x1, %rcx
    jne loop

    mov $60, %rax
    xor %rdi, %rdi
    int $0xf1

.section .rodata
msg:
    .asciz "Hello, world!"
msg_len = . - msg
