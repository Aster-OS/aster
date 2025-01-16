#pragma once

#include <stdint.h>

struct __attribute__((packed)) ioapic_t {
    uint8_t id;
    uint8_t reserved;
    uint32_t address;
    uint32_t gsi_base;
};

struct __attribute__((packed)) ioapic_iso_t {
    uint8_t bus;
    uint8_t irq;
    uint32_t gsi;
    uint16_t flags;
};

struct __attribute__((packed)) ioapic_nmi_t {
    uint16_t flags;
    uint32_t gsi;
};

struct __attribute__((packed)) lapic_nmi_t {
    uint8_t acpi_id;
    uint16_t flags;
    uint8_t lint;
};

enum madt_pin_polarity {
    MADT_ACTIVE_HIGH = 0x1,
    MADT_ACTIVE_LOW = 0x3
};

enum madt_trigger_mode {
    MADT_TRIGGER_EDGE = 0x4,
    MADT_TRIGGER_LEVEL = 0xc
};

struct lapic_nmi_t **madt_get_lapic_nmis(void);
uint16_t madt_get_lapic_nmi_count(void);

struct ioapic_t **madt_get_ioapics(void);
uint16_t madt_get_ioapic_count(void);

struct ioapic_iso_t **madt_get_ioapic_isos(void);
uint16_t madt_get_ioapic_iso_count(void);

struct ioapic_nmi_t **madt_get_ioapic_nmis(void);
uint16_t madt_get_ioapic_nmi_count(void);

struct ioapic_t *madt_find_ioapic_by_gsi(uint32_t gsi);
struct ioapic_iso_t *madt_find_iso_by_isa_irq(uint8_t isa_irq);
void madt_init(void);
