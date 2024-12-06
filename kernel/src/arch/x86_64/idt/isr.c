#include "kpanic/kpanic.h"
#include "kprintf/kprintf.h"

struct __attribute__((packed)) isr_frame_t {
    uint64_t cr4, cr3, cr2, cr0;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error_code, rip, cs, rflags, rsp, ss; // pushed by CPU
};

void isr_handler(struct isr_frame_t *frame) {
    if (frame->vector < 32) { // exception
        kprintf("CPU EXCEPTION! V 0x%x  EC 0x%llx\n", frame->vector, frame->error_code);
        kprintf("REGISTER DUMP: \n");
        kprintf("  RAX %016llx    RBX %016llx    RCX %016llx    RDX %016llx\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
        kprintf("  RDI %016llx    RSI %016llx    RBP %016llx    RSP %016llx\n", frame->rdi, frame->rsi, frame->rbp, frame->rsp);
        kprintf("  R8  %016llx    R9  %016llx    R10 %016llx    R11 %016llx\n", frame->r8, frame->r9, frame->r10, frame->r11);
        kprintf("  R12 %016llx    R13 %016llx    R14 %016llx    R15 %016llx\n", frame->r12, frame->r13, frame->r14, frame->r15);
        kprintf("  CR0 %016llx    CR2 %016llx    CR3 %016llx    CR4 %016llx\n", frame->cr0, frame->cr2, frame->cr3, frame->cr4);
        kprintf("  RIP %016llx    CS  %04llx                SS  %04llx             RFLAGS %016llx\n", frame->rip, frame->cs, frame->ss, frame->rflags);

        kpanic("Unhandled CPU Exception\n");
    } else {
        kprintf("Received interrupt 0x%x (ignored)\n", frame->vector);
    }
}
