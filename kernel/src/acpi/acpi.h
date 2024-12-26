#pragma once

#include <stdint.h>

#include "memory/pmm/pmm.h"

struct __attribute__((packed)) sdt_hdr_t {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};

uint8_t acpi_calculate_table_checksum(void *table);
struct sdt_hdr_t *acpi_find_table(char *signature);
void acpi_init(phys_t rsdp_addr);
