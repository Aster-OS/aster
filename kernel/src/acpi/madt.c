#include <stddef.h>

#include "acpi/acpi.h"
#include "acpi/madt.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "memory/vmm/vmm.h"

struct __attribute__((packed)) madt_entry_t {
    uint8_t type;
    uint8_t length;
    uint8_t start[];
};

struct __attribute__((packed)) madt_lapic_t {
    uint8_t acpi_id;
    uint8_t lapic_id;
    uint32_t flags;
};

struct __attribute__((packed)) madt_ioapic_t {
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t address;
    uint32_t gsi_base;
};

struct __attribute__((packed)) madt_ioapic_iso_t {
    uint8_t bus;
    uint8_t irq;
    uint32_t gsi;
    uint16_t flags;
};

struct __attribute__((packed)) madt_ioapic_nmi_src_t {
    uint16_t flags;
    uint32_t gsi;
};

struct __attribute__((packed)) madt_lapic_nmi_t {
    uint8_t acpi_id;
    uint16_t flags;
    uint8_t lint;
};

struct __attribute__((packed)) madt_x2lapic_t {
    uint16_t reserved;
    uint32_t x2lapic_id;
    uint32_t flags;
    uint32_t acpi_id;
};

struct __attribute__((packed)) madt_t {
    struct sdt_hdr_t hdr;
    uint32_t lapic_address;
    uint32_t flags;
    uint8_t entries[];
};

void madt_init(void) {
    struct madt_t *madt = (struct madt_t *) acpi_find_table("APIC");

    if (madt == NULL) {
        kpanic("MADT not found");
    }

    if (acpi_calculate_table_checksum((void *) madt) != 0) {
        kpanic("Invalid MADT checksum");
    }

    klog_debug("MADT entries:");
    uint32_t entries_length = madt->hdr.length - offsetof(struct madt_t, entries);

    uint32_t i = 0;
    while (i < entries_length) {
        struct madt_entry_t *entry = (struct madt_entry_t *) &madt->entries[i];

        switch (entry->type) {
            case 0: {
                struct madt_lapic_t *lapic = (struct madt_lapic_t *) &entry->start;
                klog_debug("LAPIC: acpi_id %d  lapic_id %d  flags %d",
                    lapic->acpi_id, lapic->lapic_id, lapic->flags);
            } break;
            case 1: {
                struct madt_ioapic_t *ioapic = (struct madt_ioapic_t *) &entry->start;
                klog_debug("IOAPIC: ioapic_id %d  address %016llx  gsi_base %d",
                    ioapic->ioapic_id, ioapic->address, ioapic->gsi_base);
            } break;
            case 2: {
                struct madt_ioapic_iso_t *ioapic_iso = (struct madt_ioapic_iso_t *) &entry->start;
                klog_debug("IOAPIC ISO: bus %d  irq %d  gsi %d  flags %d",
                    ioapic_iso->bus, ioapic_iso->irq, ioapic_iso->gsi, ioapic_iso->flags);
            } break;
            case 3: {
                struct madt_ioapic_nmi_src_t *ioapic_nmi_src = (struct madt_ioapic_nmi_src_t *) &entry->start;
                klog_debug("IOAPIC NMI SRC:  flags %d  gsi %d",
                    ioapic_nmi_src->flags, ioapic_nmi_src->gsi);
            } break;
            case 4: {
                struct madt_lapic_nmi_t *lapic_nmi = (struct madt_lapic_nmi_t *) &entry->start;
                klog_debug("LAPIC NMI: acpi_id %d  flags %d  lint %d",
                    lapic_nmi->acpi_id, lapic_nmi->flags, lapic_nmi->lint);
            } break;
            case 9: {
                struct madt_x2lapic_t *x2lapic = (struct madt_x2lapic_t *) &entry->start;
                klog_debug("X2LAPIC: x2lapic_id %d  flags %d  acpi_id %d",
                    x2lapic->x2lapic_id, x2lapic->flags, x2lapic->acpi_id);
            } break;
        }

        i += entry->length;
    }
}
