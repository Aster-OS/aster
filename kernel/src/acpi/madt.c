#include <stddef.h>

#include "acpi/acpi.h"
#include "acpi/madt.h"
#include "arch/x86_64/apic/ioapic.h"
#include "kassert/kassert.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "memory/kmalloc/kmalloc.h"
#include "memory/vmm/vmm.h"

struct __attribute__((packed)) madt_entry_t {
    uint8_t type;
    uint8_t length;
    uint8_t start[];
};

struct __attribute__((packed)) madt_t {
    struct sdt_hdr_t hdr;
    uint32_t lapic_address;
    uint32_t flags;
    uint8_t entries[];
};

// This flag indicates that the system has a PC-AT-compatible
// dual-8259 setup. The 8259 vectors must be disabled (that is,
// masked) when enabling the APIC.
// This flag is ignored and the PIC is disabled anyways.
static const uint32_t MADT_FLAG_PCAT_COMPAT = 1 << 0;

static struct lapic_nmi_t **lapic_nmis;
static uint16_t lapic_nmi_count;

static struct ioapic_t **ioapics;
static uint16_t ioapic_count;

static struct ioapic_iso_t **ioapic_isos;
static uint16_t ioapic_iso_count;

static struct ioapic_nmi_t **ioapic_nmis;
static uint16_t ioapic_nmi_count;

struct lapic_nmi_t **madt_get_lapic_nmis(void) {
    return lapic_nmis;
}

uint16_t madt_get_lapic_nmi_count(void) {
    return lapic_nmi_count;
}

struct ioapic_t **madt_get_ioapics(void) {
    return ioapics;
}

uint16_t madt_get_ioapic_count(void) {
    return ioapic_count;
}

struct ioapic_iso_t **madt_get_ioapic_isos(void) {
    return ioapic_isos;
}

uint16_t madt_get_ioapic_iso_count(void) {
    return ioapic_iso_count;
}

struct ioapic_nmi_t **madt_get_ioapic_nmis(void) {
    return ioapic_nmis;
}

uint16_t madt_get_ioapic_nmi_count(void) {
    return ioapic_nmi_count;
}

struct ioapic_t *madt_find_ioapic_by_gsi(uint32_t gsi) {
    for (uint8_t i = 0; i < ioapic_count; i++) {
        struct ioapic_t *ioapic = ioapics[i];
        uint32_t first_gsi = ioapic->gsi_base;
        uint32_t last_gsi = first_gsi + ioapic_get_max_redir_entry(ioapic->address);
        if (first_gsi <= gsi && gsi <= last_gsi) {
            return ioapic;
        }
    }

    kpanic("Could not find IOAPIC handling GSI %llu", gsi);
}

struct ioapic_iso_t *madt_find_iso_by_isa_irq(uint8_t isa_irq) {
    return ioapic_isos[isa_irq];
}

void madt_init(void) {
    struct madt_t *madt = (struct madt_t *) acpi_find_table("APIC");

    kassert(madt != NULL);
    kassert(acpi_calculate_table_checksum(madt) == 0);

    klog_debug("Dual 8259 PIC system: %s", madt->flags & MADT_FLAG_PCAT_COMPAT ? "yes" : "no");

    klog_debug("MADT entries:");
    uint32_t entries_length = madt->hdr.length - offsetof(struct madt_t, entries);

    uint32_t i = 0;
    while (i < entries_length) {
        struct madt_entry_t *entry = (struct madt_entry_t *) &madt->entries[i];

        if (entry->type == 1) { // IOAPIC
            ioapic_count++;
        } else if (entry->type == 2) { // IOAPIC ISO
            ioapic_iso_count++;
        } else if (entry->type == 3) { // IOAPIC NMI
            ioapic_nmi_count++;
        } else if (entry->type == 4) { // LAPIC NMI
            lapic_nmi_count++;
        }

        i += entry->length;
    }

    klog_debug("MADT: Found %llu IOAPIC %s", ioapic_count, ioapic_count == 1 ? "entry" : "entries");
    klog_debug("MADT: Found %llu IOAPIC ISO %s", ioapic_iso_count, ioapic_iso_count == 1 ? "entry" : "entries");
    klog_debug("MADT: Found %llu IOAPIC NMI %s", ioapic_nmi_count, ioapic_nmi_count == 1 ? "entry" : "entries");
    klog_debug("MADT: Found %llu LAPIC NMI %s", lapic_nmi_count, lapic_nmi_count == 1 ? "entry" : "entries");

    ioapics = kmalloc(ioapic_count * sizeof(struct ioapic_t *));
    ioapic_isos = kmalloc(ioapic_iso_count * sizeof(struct ioapic_iso_t *));
    ioapic_nmis = kmalloc(ioapic_nmi_count * sizeof(struct ioapic_nmi_t *));
    lapic_nmis = kmalloc(ioapic_nmi_count * sizeof(struct lapic_nmi_t *));

    uint16_t ioapics_curr_index = 0;
    uint16_t ioapic_isos_curr_index = 0;
    uint16_t ioapic_nmis_curr_index = 0;
    uint16_t lapic_nmis_curr_index = 0;

    i = 0;
    while (i < entries_length) {
        struct madt_entry_t *entry = (struct madt_entry_t *) &madt->entries[i];

        if (entry->type == 1) { // IOAPIC
            struct ioapic_t *ioapic = (struct ioapic_t *) &entry->start;
            ioapics[ioapics_curr_index++] = ioapic;
            klog_debug("- IOAPIC: ioapic_id %llu  address %llx  gsi_base %llu", ioapic->id, ioapic->address, ioapic->gsi_base);
        
        } else if (entry->type == 2) { // IOAPIC ISO
            struct ioapic_iso_t *ioapic_iso = (struct ioapic_iso_t *) &entry->start;
            ioapic_isos[ioapic_isos_curr_index++] = ioapic_iso;
            klog_debug("- IOAPIC ISO: irq %llu  gsi %llu  flags %llu", ioapic_iso->irq, ioapic_iso->gsi, ioapic_iso->flags);

        } else if (entry->type == 3) { // IOAPIC NMI
            struct ioapic_nmi_t *ioapic_nmi = (struct ioapic_nmi_t *) &entry->start;
            ioapic_nmis[ioapic_nmis_curr_index++] = ioapic_nmi;
            klog_debug("- IOAPIC NMI: flags %llu  gsi %llu", ioapic_nmi->flags, ioapic_nmi->gsi);
        
        } else if (entry->type == 4) { // LAPIC NMI
            struct lapic_nmi_t *lapic_nmi = (struct lapic_nmi_t *) &entry->start;
            lapic_nmis[lapic_nmis_curr_index++] = lapic_nmi;
            klog_debug("- LAPIC NMI: acpi_id %llu  flags %llu  lint %llu", lapic_nmi->acpi_id, lapic_nmi->flags, lapic_nmi->lint);
        }

        i += entry->length;
    }

    klog_info("MADT initialized");
}
