#include "acpi/acpi.h"
#include "kpanic/kpanic.h"
#include "kprintf/kprintf.h"
#include "lib/string.h"
#include "limine.h"
#include "memory/vmm/vmm.h"

struct __attribute__((packed)) xsdp_t {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
};

struct __attribute__((packed)) xsdt_t {
    struct sdt_header_t header;
    phys_t entries[];
};

static struct xsdt_t *xsdt;
static uint64_t xsdt_entry_count;

uint8_t acpi_calculate_table_checksum(phys_t table) {
    struct sdt_header_t *header = (struct sdt_header_t *) table;

    // when calculating the checksum, only the last byte matters
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < header->length; i++) {
        checksum += ((uint8_t *) table)[i];
    }

    return checksum;
}

struct sdt_header_t *acpi_get_table(char *signature) {
    for (uint64_t i = 0; i < xsdt_entry_count; i++) {
        struct sdt_header_t *entry_header = (struct sdt_header_t *) (xsdt->entries[i] + vmm_get_hhdm_offset());
        if (strncmp(entry_header->signature, signature, 4) == 0) {
            return entry_header;
        }
    }
    
    return NULL;
}

void acpi_init(phys_t rsdp_address) {
    vmm_map_page(vmm_get_kernel_pagemap(), rsdp_address + vmm_get_hhdm_offset(), rsdp_address, PTE_FLAGS_HHDM);
    struct xsdp_t *xsdp = (struct xsdp_t *) (rsdp_address + vmm_get_hhdm_offset());

    if (xsdp->revision < 2) {
        kpanic("Unsupported RSDP revision (%d)\n", xsdp->revision);
    }

    uint8_t checksum = 0;
    for (uint32_t i = 0; i < sizeof(struct xsdp_t); i++) {
        checksum += ((uint8_t *) xsdp)[i];
    }

    if (checksum != 0) {
        kpanic("Invalid XSDP checksum\n");
    }

    vmm_map_page(vmm_get_kernel_pagemap(), xsdp->xsdt_address + vmm_get_hhdm_offset(), xsdp->xsdt_address, PTE_FLAGS_HHDM);
    xsdt = (struct xsdt_t *) (xsdp->xsdt_address + vmm_get_hhdm_offset());

    if (acpi_calculate_table_checksum((phys_t) xsdt) != 0) {
        kpanic("Invalid XSDT checksum\n");
    }

    xsdt_entry_count = (xsdt->header.length - offsetof(struct xsdt_t, entries)) / 8;

    for (uint32_t i = 0; i < xsdt_entry_count; i++) {
        phys_t entry_address = xsdt->entries[i];
        vmm_map_page(vmm_get_kernel_pagemap(), entry_address + vmm_get_hhdm_offset(), entry_address, PTE_FLAGS_HHDM);
    }

    kprintf("ACPI initialized\n");
}
