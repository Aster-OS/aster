#include "arch/x86_64/gdt/gdt-sel.h"

.section .text

.global gdt_reload_seg_internal
gdt_reload_seg_internal:
    lgdt (%rdi)

    push $GDT_SEL_KCODE
    lea reload_data_segments(%rip), %rax
    push %rax
    lretq

reload_data_segments:
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    ret
