#include <stddef.h>

#include "acpi/madt.h"
#include "arch/x86_64/apic/ioapic.h"
#include "arch/x86_64/interrupts/interrupts.h"
#include "kassert/kassert.h"
#include "klog/klog.h"
#include "memory/vmm/vmm.h"
#include "mp/mp.h"

static const uint32_t IOREGSEL = 0x0;
static const uint32_t IOWIN = 0x10;

enum ioapic_regs {
    IOAPICID = 0x0,
    IOAPICVER = 0x1,
    IOAPICARB = 0x2,
    IOREDTBL = 0x10
};

static uint32_t ioapic_rd(uint32_t ioapic_addr, uint8_t reg) {
    *(volatile uint32_t *) (ioapic_addr + IOREGSEL + vmm_get_hhdm_offset()) = reg;
    return *(volatile uint32_t *) (ioapic_addr + IOWIN + vmm_get_hhdm_offset());
}

static void ioapic_wr(uint32_t ioapic_addr, uint8_t reg, uint32_t val) {
    *(volatile uint32_t *) (ioapic_addr + IOREGSEL + vmm_get_hhdm_offset()) = reg;
    *(volatile uint32_t *) (ioapic_addr + IOWIN + vmm_get_hhdm_offset()) = val;
}

uint32_t ioapic_get_max_redir_entry(uint32_t ioapic_addr) {
    return (ioapic_rd(ioapic_addr, IOAPICVER) >> 16) & 0xff;
}

void ioapic_wr_redtbl(struct ioapic_t *ioapic, uint32_t gsi, uint64_t val) {
    uint32_t relative_gsi = gsi - ioapic->gsi_base;
    uint32_t redir_entry_reg_lo = IOREDTBL + 2 * relative_gsi;
    uint32_t redir_entry_reg_hi = IOREDTBL + 2 * relative_gsi + 1;
    ioapic_wr(ioapic->address, redir_entry_reg_lo, val & 0xffffffff);
    ioapic_wr(ioapic->address, redir_entry_reg_hi, val >> 32);
}

void ioapic_init(void) {
    for (uint16_t i = 0; i < madt_get_ioapic_count(); i++) {
        struct ioapic_t *ioapic = madt_get_ioapics()[i];
        vmm_map_hhdm(ioapic->address);

        uint32_t first_gsi = ioapic->gsi_base;
        uint32_t last_gsi = first_gsi + ioapic_get_max_redir_entry(ioapic->address);

        klog_info("IOAPIC id %llu initialized (GSIs %llu-%llu)", ioapic->id, first_gsi, last_gsi);
    }
}

void ioapic_unmask_isa_irq(uint8_t isa_irq) {
    kassert(isa_irq < ISA_IRQ_COUNT);

    // TODO: GSI/IRQ balancing...
    uint8_t cpu_handling_irq_lapic_id = mp_get_bsp()->lapic_id;
    uint8_t isa_irq_vec = interrupts_get_isa_irq_vec(isa_irq);
    struct ioapic_iso_t *ioapic_iso = madt_find_iso_by_isa_irq(isa_irq);

    if (ioapic_iso == NULL) { // identity map ISA IRQ to GSI
        uint32_t gsi = isa_irq;

        struct ioapic_t *ioapic = madt_find_ioapic_by_gsi(gsi);

        uint64_t redtbl_val = 0;
        redtbl_val |= isa_irq_vec;
        redtbl_val |= IOAPIC_DELIV_MODE_FIXED;
        redtbl_val |= IOAPIC_DEST_MODE_PHYSICAL;
        redtbl_val |= IOAPIC_ACTIVE_HIGH;
        redtbl_val |= IOAPIC_TRIGGER_EDGE;
        redtbl_val |= ioapic_dest(cpu_handling_irq_lapic_id);

        ioapic_wr_redtbl(ioapic, gsi, redtbl_val);
    } else {
        uint32_t gsi = ioapic_iso->gsi;

        struct ioapic_t *ioapic = madt_find_ioapic_by_gsi(gsi);

        uint64_t redtbl_val = 0;
        redtbl_val |= isa_irq_vec;

        uint16_t iso_flags = ioapic_iso->flags;
        if ((iso_flags & MADT_ACTIVE_HIGH) == MADT_ACTIVE_HIGH) {
            redtbl_val |= IOAPIC_ACTIVE_HIGH;
        } else if ((iso_flags & MADT_ACTIVE_LOW) == MADT_ACTIVE_LOW) {
            redtbl_val |= IOAPIC_ACTIVE_LOW;
        }

        if ((iso_flags & MADT_TRIGGER_EDGE) == MADT_TRIGGER_EDGE) {
            redtbl_val |= IOAPIC_TRIGGER_EDGE;
        } else if ((iso_flags & MADT_TRIGGER_LEVEL) == MADT_TRIGGER_LEVEL) {
            redtbl_val |= IOAPIC_TRIGGER_LEVEL;
        }

        redtbl_val |= ioapic_dest(cpu_handling_irq_lapic_id);
        ioapic_wr_redtbl(ioapic, gsi, redtbl_val);
    }

    klog_debug("IOAPIC: Unmasked ISA IRQ %llu", isa_irq);
}
