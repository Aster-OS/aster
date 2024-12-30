#include <stdint.h>

#include "arch/x86_64/apic/pic.h"
#include "arch/x86_64/asm_wrappers.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "klog/klog.h"

static const uint16_t PIC1_COMMAND_PORT = 0x20;
static const uint16_t PIC1_DATA_PORT = 0x21;
static const uint16_t PIC2_COMMAND_PORT = 0xa0;
static const uint16_t PIC2_DATA_PORT = 0xa1;

// Slave PIC was connected to the master PIC via IRQ 2 on the master PIC
static const uint8_t IRQ_SLAVE_PIC_TO_MASTER_PIC = 2;

static const uint8_t ICW1_ICW4_PRESENT = 0x01;
static const uint8_t ICW1_INIT = 0x10;
static const uint8_t ICW4_8086 = 0x01;

void pic_disable(void) {
    // ICW1: Start initialization in cascade mode
    outb(PIC1_COMMAND_PORT, ICW1_INIT | ICW1_ICW4_PRESENT);
    outb(PIC2_COMMAND_PORT, ICW1_INIT | ICW1_ICW4_PRESENT);

    // ICW2: Set vector offset (remap IRQ's)
    outb(PIC1_DATA_PORT, PIC1_VECTOR_OFFSET);
    outb(PIC2_DATA_PORT, PIC2_VECTOR_OFFSET);

    // ICW3: Slave PIC is connected to master PIC via IRQ 2
    outb(PIC1_DATA_PORT, 1 << IRQ_SLAVE_PIC_TO_MASTER_PIC);
    outb(PIC2_DATA_PORT, IRQ_SLAVE_PIC_TO_MASTER_PIC);

    // ICW4: Use 8086 mode
    outb(PIC1_DATA_PORT, ICW4_8086);
    outb(PIC2_DATA_PORT, ICW4_8086);

    // Mask all IRQ's
    outb(PIC1_DATA_PORT, 0xff);
    outb(PIC2_DATA_PORT, 0xff);    

    klog_info("PIC disabled");
}
