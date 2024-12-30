#include "klog/klog.h"
#include "kpanic/kpanic.h"

struct __attribute__((packed)) isr_frame_t {
    uint64_t cr4, cr3, cr2, cr0;
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error_code, rip, cs, rflags, rsp, ss; // pushed by CPU
};

void isr_handler(struct isr_frame_t *frame) {
    if (frame->vector < 32) { // exception
        kpanic("Unhandled CPU Exception");
    } else {
        klog_warn("Received interrupt 0x%x (ignored)", frame->vector);
    }
}
