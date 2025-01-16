#pragma once

#include "acpi/madt.h"

enum ioapic_redtbl_deliv_mode {
    IOAPIC_DELIV_MODE_FIXED     = 0x0,
    IOAPIC_DELIV_MODE_LOW_PRIOR = 0x100,
    IOAPIC_DELIV_MODE_SMI       = 0x200,
    IOAPIC_DELIV_MODE_NMI       = 0x400,
    IOAPIC_DELIV_MODE_INIT      = 0x500,
    IOAPIC_DELIV_MODE_EXTINT    = 0x700
};

enum ioapic_redtbl_dest_mode {
    IOAPIC_DEST_MODE_PHYSICAL = 0x0,
    IOAPIC_DEST_MODE_LOGICAL  = 0x800
};

enum ioapic_deliv_status {
    IOAPIC_DELIV_STATUS_SENT    = 0x0,
    IOAPIC_DELIV_STATUS_PENDING = 0x1000,
};

enum ioapic_redtbl_pin_polarity {
    IOAPIC_ACTIVE_HIGH = 0x0,
    IOAPIC_ACTIVE_LOW  = 0x2000
};

enum ioapic_redtbl_trigger_mode {
    IOAPIC_TRIGGER_EDGE  = 0x0,
    IOAPIC_TRIGGER_LEVEL = 0x8000
};

#define IOAPIC_MASKED 0x10000

static inline uint64_t ioapic_dest(uint8_t lapic_id) {
    return (uint64_t) lapic_id << 56;
}

uint32_t ioapic_get_max_redir_entry(uint32_t ioapic_addr);
void ioapic_init(void);
void ioapic_wr_redtbl(struct ioapic_t *ioapic, uint32_t gsi, uint64_t val);
void ioapic_unmask_isa_irq(uint8_t isa_irq);
