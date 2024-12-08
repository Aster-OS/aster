#pragma once

#include <stdint.h>

struct __attribute__((packed)) sdt_header_t {
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
struct sdt_header_t *acpi_get_table(char *signature);
void acpi_init(uint64_t rsdp_address);
