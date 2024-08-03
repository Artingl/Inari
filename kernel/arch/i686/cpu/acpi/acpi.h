#pragma once

#include <kernel/include/C/typedefs.h>
#include <kernel/arch/i686/cpu/cpu.h>

#define APIC_LOCAL_PROCESSOR 0
#define APIC_IO 1
#define APIC_IO_INT_SRC_OVERRIDE 2
#define APIC_IO_NON_MASK_INT_SRC 3
#define APIC_LOCAL_NON_MASK_INT 4
#define APIC_LOCAL_ADDR_OVERRIDE 5
#define APIC_LOCAL_PROCESSOR_x2 9

/* Iterate through all ACPI SDTs
 *
 * This macro will:
 *  1. Get the data size of a pointer to the entry inside RSDT_t.sdt_pointers
 *     array and the length of the array.
 *  2. Get the array element depending on the data size
 */
#define ACPI_ITERATE(idx, ptr, statement)                                           \
    {                                                                               \
        struct XSDP *sdp = cpu_acpi_sdp();                                          \
        struct XSDT *root_sdt = cpu_acpi_root_sdt();                                \
        size_t idx, len = (root_sdt->header.length - sizeof(root_sdt->header)) /    \
                          (sdp->revision == 2 ? 8 : 4);                             \
        struct SDT *ptr;                                                            \
        for (idx = 0; idx < len; idx++)                                             \
        {                                                                           \
            if (sdp->revision == 2)                                                 \
                ptr = (struct SDT *)(uintptr_t)(root_sdt->sdt_pointers[idx]);       \
            else                                                                    \
                ptr = (struct SDT *)(((struct RSDT *)root_sdt)->sdt_pointers[idx]); \
            kident(ptr, PAGE_SIZE, KERN_PAGE_RW);                                   \
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

struct FADT_genaddr
{
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
};

struct FADT
{
    struct SDT header;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t reserved;

    uint8_t preferred_power_management_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_control;
    uint16_t worstc2_latency;
    uint16_t worstc3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t boot_architecture_flags;

    uint8_t reserved2;
    uint32_t flags;

    // 12 byte structure; see below for details
    struct FADT_genaddr reset_reg;

    uint8_t reset_value;
    uint8_t reserved3[3];

    // 64bit pointers - Available on ACPI 2.0+
    uint64_t x_firmware_control;
    uint64_t x_dsdt;

    struct FADT_genaddr x_pm1_aevent_block;
    struct FADT_genaddr x_pm1_bevent_block;
    struct FADT_genaddr x_pm1_control_block;
    struct FADT_genaddr x_pm1_bcontrol_block;
    struct FADT_genaddr x_pm2_control_block;
    struct FADT_genaddr x_pm_timer_block;
    struct FADT_genaddr x_gpe0_block;
    struct FADT_genaddr x_gpe1_block;
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

int cpu_acpi_init();

struct XSDP *cpu_acpi_sdp();
struct XSDT *cpu_acpi_root_sdt();

int cpu_acpi_poweroff();
int cpu_acpi_reboot();
int cpu_acpi_loaded();

uint32_t cpu_acpi_remap_irq(uint32_t irq);

uint8_t cpu_acpi_load_madt(struct cpu_core *cores);
