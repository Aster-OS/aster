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
        kprintf("CPU EXCEPTION! V=0x%x EC=0x%llx\n", frame->vector, frame->error_code);
        kprintf("REGISTER DUMP\n");
        kprintf("  RAX=0x%016llx    RBX=0x%016llx    RCX=0x%016llx    RDX=0x%016llx\n", frame->rax, frame->rbx, frame->rcx, frame->rdx);
        kprintf("  RDI=0x%016llx    RSI=0x%016llx    RBP=0x%016llx    RSP=0x%016llx\n", frame->rdi, frame->rsi, frame->rbp, frame->rsp);
        kprintf("  R8 =0x%016llx    R9 =0x%016llx    R10=0x%016llx    R11=0x%016llx\n", frame->r8, frame->r9, frame->r10, frame->r11);
        kprintf("  R12=0x%016llx    R13=0x%016llx    R14=0x%016llx    R15=0x%016llx\n", frame->r12, frame->r13, frame->r14, frame->r15);
        kprintf("  CR0=0x%016llx    CR2=0x%016llx    CR3=0x%016llx    CR4=0x%016llx\n", frame->cr0, frame->cr2, frame->cr3, frame->cr4);
        kprintf("  RIP=0x%016llx    CS =0x%04llx                SS =0x%04llx             RFLAGS=0x%016llx\n", frame->rip, frame->cs, frame->ss, frame->rflags);

        kpanic("Unhandled CPU Exception");
    } else {
        kprintf("Received interrupt V=0x%x (ignored)\n", frame->vector);
    }
}
