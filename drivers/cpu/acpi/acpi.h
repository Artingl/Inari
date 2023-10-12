#pragma once

#include <kernel/include/C/typedefs.h>

#define APIC_LOCAL_PROCESSOR 0
#define APIC_IO 1
#define APIC_IO_INT_SRC_OVERRIDE 2
#define APIC_IO_NON_MASK_INT_SRC 3
#define APIC_LOCAL_NON_MASK_INT 4
#define APIC_LOCAL_ADDR_OVERRIDE 5
#define APIC_LOCAL_PROCESSOR_x2 9

/* Iterate thru all ACPI SDTs
 *
 * this macro will:
 *  1. get the data size of pointer to the entry inside RSDT_t.sdt_pointers array and the length of the array.
 *  2. get the array element depending on the data size
 */
#define ACPI_ITERATE(idx, ptr, statement)                                           \
    {                                                                               \
        struct XSDP *sdp = cpu_acpi_sdp();                                     \
        struct XSDT *root_sdt = cpu_acpi_root_sdt();                           \
        size_t idx, len = (root_sdt->header.length - sizeof(struct SDT)) /          \
                          (sdp->revision == 2 ? 8 : 4);                             \
        struct SDT *ptr;                                                            \
        for (idx = 0; idx < len; idx++)                                             \
        {                                                                           \
            struct SDT *ptr;                                                        \
            kident(ptr, PAGE_SIZE, KERN_PAGE_RW);                                   \
            if (sdp->revision == 2)                                                 \
                ptr = (struct SDT *)(root_sdt->sdt_pointers[idx]);                  \
            else                                                                    \
                ptr = (struct SDT *)(((struct RSDT *)root_sdt)->sdt_pointers[idx]); \
            statement                                                               \
        }                                                                           \
    }

struct RSDP
{
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed));

struct XSDP
{
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address; // deprecated since version 2.0

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct SDT
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oemtableid[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};

struct MADT
{
    struct SDT header;
    uint32_t local_apic_address;
    uint32_t flags;
};

struct RSDT
{
    struct SDT header;
    uint32_t sdt_pointers[];
};

struct XSDT
{
    struct SDT header;
    uint64_t sdt_pointers[];
};

struct MADT_Entry
{
    uint8_t entry_type;
    uint8_t record_length;
};

struct IOAPIC_MADT
{
    struct MADT_Entry header;

    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_int_base;
};

// IO/APIC Interrupt Source Override
struct IOAPIC_INTSRCO_MADT
{
    struct MADT_Entry header;

    uint8_t bus;
    uint8_t irq;
    uint32_t gsi;
    uint16_t flags;
};

struct LAPIC_MADT
{
    struct MADT_Entry header;

    uint8_t acpi_proc_id;
    uint8_t apic_id;
    uint32_t flags;
};

void cpu_acpi_init();
bool cpu_acpi_signature(void *ptr, size_t size);

struct XSDP *cpu_acpi_sdp();
struct XSDT *cpu_acpi_root_sdt();

bool cpu_acpi_shutdown();
bool cpu_acpi_reboot();
bool cpu_acpi_loaded();

uint32_t cpu_acpi_remap_irq(uint32_t irq);

uint8_t cpu_acpi_load_madt(uintptr_t *lapic, uintptr_t *ioapic);
