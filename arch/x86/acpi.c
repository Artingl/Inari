#include <kernel/inari.h>
#include <kernel/errno.h>
#include <kernel/mm/pmm.h>

#include <misc/string.h>

#include <arch/paging.h>
#include <arch/x86/acpi.h>
#include <arch/x86/cpu.h>

#define APIC_LOCAL_PROCESSOR 0
#define APIC_IO 1
#define APIC_IO_INT_SRC_OVERRIDE 2
#define APIC_IO_NON_MASK_INT_SRC 3
#define APIC_LOCAL_NON_MASK_INT 4
#define APIC_LOCAL_ADDR_OVERRIDE 5
#define APIC_LOCAL_PROCESSOR_x2 9

static struct XSDP *sdp = NULL;
static struct XSDT *root_sdt = NULL;
static uintptr_t acpi_cpu_ioapic = (uintptr_t)NULL;
static uint32_t acpi_cpu_count;
static int acpi_initialized = 0;

static int acpi_signature(void *ptr, size_t size)
{
    uint32_t checksum = 0, i;
    for (i = 0; i < size; i++)
        checksum += *(((char *)ptr) + i);
    return (checksum & 0xf) == 0;
}

int x86_acpi_init(void)
{
    size_t i;

    if (!(x86_cpu_features_edx() & CPU_FEATURE_EDX_ACPI))
    {
        kprintf("acpi: not found.");
        return -ENODEV;
    }

    /* Try to find the signature of RSDP */
    for (i = 0x000E0000; i < 0x000FFFFF; i+=8)
    {
        if (memcmp((char *)i, "RSD PTR ", 8) == 0)
        {
            sdp = (struct XSDP *)i;
            break;
        }
    }

    /* TODO: also perform search in EBDA (https://wiki.osdev.org/RSDP) */
    if (!sdp)
    {
        kprintf("acpi: unable to find RSDP in memory.");
        return -ENODEV;
    }

    if (sdp->revision == 0)
    {
        if (!(acpi_signature(sdp, sizeof(struct RSDP))))
        {
            kprintf("acpi: RSDP invalid checksum.");
            return -EINVAL;
        }

        kprintf("acpi: SDP version 1.0==");
        root_sdt = (struct XSDT *)sdp->rsdt_address;
    }
    else if (sdp->revision == 2)
    {
        if (!(acpi_signature(sdp, sizeof(struct XSDP))))
        {
            kprintf("acpi: XSDP invalid checksum.");
            return -EINVAL;
        }

        kprintf("acpi: SDP version 2.0>=");
        root_sdt = (struct XSDT *)(uint32_t)sdp->xsdt_address;
        if (!root_sdt)
            root_sdt = (struct XSDT *)sdp->rsdt_address;
    }
    else {
        kprintf("acpi: invalid SDP revision.");
        return -EINVAL;
    }

    pmm_reserve_memory((struct reserved_memory){
        .start = (uintptr_t)sdp,
        .end = (uintptr_t)sdp + PAGE_SIZE
    });
    pmm_reserve_memory((struct reserved_memory){
        .start = (uintptr_t)root_sdt,
        .end = (uintptr_t)root_sdt + PAGE_SIZE
    });

    arch_map_page(arch_get_kernel_pagedir(), root_sdt, root_sdt, PAGE_SIZE, PAGE_RW | PAGE_PRESENT);

    if (root_sdt->header.length == 0)
    {
        kprintf("acpi: r/xSDT invalid header length.");
        return -EINVAL;
    }
    else if (!(acpi_signature(root_sdt, root_sdt->header.length)))
    {
        panic("acpi: RootSDT invalid checksum.");
        return -EINVAL;
    }

    acpi_initialized = 1;
    ACPI_ITERATE(idx, base, {
        pmm_reserve_memory((struct reserved_memory){
            .start = (uintptr_t)base,
            .end = (uintptr_t)base + PAGE_SIZE
        });
    })

    return 0;
}

struct XSDP *x86_acpi_sdp(void)
{
    if (!acpi_initialized)
        return (struct XSDP *)NULL;
    return sdp;
}

struct XSDT *x86_acpi_root_sdt(void)
{
    if (!acpi_initialized)
        return (struct XSDT *)NULL;
    return root_sdt;
}

uint32_t x86_acpi_cpu_count(void)
{
    if (!acpi_initialized)
        return 1;
    return acpi_cpu_count;
}

uintptr_t x86_acpi_cpu_ioapic(void)
{
    if (!acpi_initialized)
        return 1;
    return acpi_cpu_ioapic;
}

int x86_acpi_load_madt(struct x86_cpu *cpus_pool)
{
    if (!acpi_initialized)
        goto err;
    
    struct MADT_Entry *entry;
    size_t madts = 0;
    uint8_t *i;
    acpi_cpu_count = 0;

    /* Iterate thru all SDTs to find MADT */
    ACPI_ITERATE(idx, pointer, {
        if (memcmp(pointer->signature, "APIC", 4) == 0)
        {
            madts++;

            /* Iterate thru all MADT entries to get all APICs on the system */
            struct MADT *madt = (struct MADT *)pointer;

            if (!madt || !acpi_signature(madt, madt->header.length))
                panic("acpi: unable to find MADT record; or got invalid record (bad checksum)");

            /* Iterate thru all MADT entries to get all APICs on the system */
            for (
                i = ((uint8_t *)(madt)) + 0x2c;
                i < ((uint8_t *)(madt)) + madt->header.length + 0x2c;)
            {
                entry = (struct MADT_Entry*)(i);
                if (entry->record_length <= 0)
                    break;

                if (entry->entry_type == APIC_LOCAL_PROCESSOR)
                {
                    struct LAPIC_MADT *lapic_table = (struct LAPIC_MADT *)(entry);

                    if (acpi_cpu_count + 1 >= CONFIG_MAX_CORES)
                    {
                        kprintf("acpi: ignoring out-of-bounds core id %u", lapic_table->acpi_proc_id);
                        goto next;
                    }

                    cpus_pool[lapic_table->acpi_proc_id].is_available = 1;
                    cpus_pool[lapic_table->acpi_proc_id].core_id = lapic_table->acpi_proc_id;
                    cpus_pool[lapic_table->acpi_proc_id].lapic_id = lapic_table->apic_id;
                    cpus_pool[lapic_table->acpi_proc_id].lapic_ptr = madt->local_apic_address;

                    acpi_cpu_count++;
                    kprintf("acpi: lapic found[%d]", lapic_table->acpi_proc_id);
                }
                else if (entry->entry_type == APIC_IO)
                {
                    struct IOAPIC_MADT *io_apic_table = (struct IOAPIC_MADT *)(entry);

                    /* TODO: only one IO/APIC can be used right now */
                    if (acpi_cpu_ioapic)
                    {
                        kprintf("acpi: another io/apic was found, which is not supported");
                        goto next;
                    }

#ifdef CONFIG_DEBUG
                    kprintf("acpi: io/apic found[%d]: 0x%x", io_apic_table->io_apic_id, io_apic_table->io_apic_address);
#endif
                    acpi_cpu_ioapic = io_apic_table->io_apic_address;
                }
                else if (entry->entry_type == APIC_IO_INT_SRC_OVERRIDE)
                {
#ifdef CONFIG_DEBUG
                    struct IOAPIC_INTSRCO_MADT *int_overrd = (struct IOAPIC_INTSRCO_MADT *)(entry);
                    kprintf("acpi: io/apic ISO: bus = 0x%x, irq = 0x%x, gsi = 0x%x, flags = 0x%x",
                        int_overrd->bus, int_overrd->irq, int_overrd->gsi, int_overrd->flags);
#endif
                }
                
            next:
                i += entry->record_length;
            }
        }
    })

    return 0;
err:
    acpi_cpu_ioapic = (uintptr_t)NULL;
    acpi_cpu_count = 1;

    return -ENODEV;
}
