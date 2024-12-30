#pragma once

// Each PIC provides 8 IRQs
#define PIC_IRQ_COUNT 8

// PIC IRQs will be remapped after the exception vectors
#define PIC1_VECTOR_OFFSET 0x20
#define PIC2_VECTOR_OFFSET 0x28

void pic_disable(void);
