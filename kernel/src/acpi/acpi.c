#include "acpi/acpi.h"
#include "klog/klog.h"
#include "kpanic/kpanic.h"
#include "lib/string.h"
#include "limine.h"
#include "memory/vmm/vmm.h"

struct __attribute__((packed)) rsdp_t {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
};

struct __attribute__((packed)) xsdp_t {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    // The following fields exist only in ACPI v2+
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
};

struct __attribute__((packed)) rsdt_t {
    struct sdt_hdr_t hdr;
    uint32_t entries[];
};

struct __attribute__((packed)) xsdt_t {
    struct sdt_hdr_t hdr;
    uint64_t entries[];
};

static bool xsdt_supported;
static void *rsdt_or_xsdt;
static uint64_t acpi_table_count;

uint8_t acpi_calculate_table_checksum(void *table) {
    struct sdt_hdr_t *hdr = (struct sdt_hdr_t *) table;

    // Only the last byte of the checksum matters
    uint8_t checksum = 0;
    for (uint32_t i = 0; i < hdr->length; i++) {
        checksum += ((uint8_t *) table)[i];
    }

    return checksum;
}

struct sdt_hdr_t *acpi_find_table(char *signature) {
    for (uint64_t i = 0; i < acpi_table_count; i++) {
        phys_t table_addr;
        if (xsdt_supported) {
            table_addr = ((struct xsdt_t *) rsdt_or_xsdt)->entries[i];
        } else {
            table_addr = ((struct rsdt_t *) rsdt_or_xsdt)->entries[i];
        }

        // ACPI tables are mapped in the HHDM in initialization
        struct sdt_hdr_t *table_hdr = (struct sdt_hdr_t *) (table_addr + vmm_get_hhdm_offset());
        if (strncmp(table_hdr->signature, signature, 4) == 0) {
            return table_hdr;
        }
    }
    
    return NULL;
}

void acpi_init(phys_t rsdp_addr) {
    vmm_map_hhdm(rsdp_addr);
    struct rsdp_t *rsdp = (struct rsdp_t *) (rsdp_addr + vmm_get_hhdm_offset());
    struct xsdp_t *xsdp = (struct xsdp_t *) rsdp;

    klog_debug("ACPI revision %d", rsdp->revision);

    phys_t rsdt_or_xsdt_addr;
    if (rsdp->revision < 2) {
        rsdt_or_xsdt_addr = rsdp->rsdt_address;
        xsdt_supported = false;
    } else {
        // ACPI version 2+, use the XSDT address instead
        rsdt_or_xsdt_addr = xsdp->xsdt_address;
        xsdt_supported = true;
    }

    // Only the last byte of the checksum matters
    uint8_t checksum = 0;
    uint32_t rsdp_or_xsdp_sz = xsdt_supported ? sizeof(struct xsdp_t) : sizeof(struct rsdp_t);
    for (uint32_t i = 0; i < rsdp_or_xsdp_sz; i++) {
        // RSDP and XSDP point to the same addr, either can be used here
        // the above calculated size is the one that matters
        checksum += ((uint8_t *) rsdp)[i];
    }

    if (checksum != 0) {
        kpanic("Invalid %s checksum", xsdt_supported ? "XSDP" : "RSDP");
    }

    vmm_map_hhdm(rsdt_or_xsdt_addr);
    rsdt_or_xsdt = (void *) (rsdt_or_xsdt_addr + vmm_get_hhdm_offset());

    if (acpi_calculate_table_checksum(rsdt_or_xsdt) != 0) {
        kpanic("Invalid XSDT checksum");
    }

    if (xsdt_supported) {
        struct xsdt_t *xsdt = (struct xsdt_t *) rsdt_or_xsdt;
        acpi_table_count = (xsdt->hdr.length - sizeof(struct sdt_hdr_t)) / 8;
    } else {
        struct rsdt_t *rsdt = (struct rsdt_t *) rsdt_or_xsdt;
        acpi_table_count = (rsdt->hdr.length - sizeof(struct sdt_hdr_t)) / 4;
    }

    for (uint64_t i = 0; i < acpi_table_count; i++) {
        phys_t table_addr;
        if (xsdt_supported) {
            table_addr = ((struct xsdt_t *) rsdt_or_xsdt)->entries[i];
        } else {
            table_addr = ((struct rsdt_t *) rsdt_or_xsdt)->entries[i];
        }
        vmm_map_hhdm(table_addr);
    }

    klog_info("ACPI initialized");
}
