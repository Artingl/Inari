#include <kernel/kernel.h>

#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/cpu.h>

#include <drivers/memory/vmm.h>

#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/string.h>

struct XSDP *sdp = NULL;
struct XSDT *root_sdt = NULL;

bool acpi_loaded = false;

void cpu_acpi_init()
{
    size_t i;

    if (!(cpu_feat_edx() & CPU_FEATURE_EDX_ACPI))
    {
        printk(KERN_NOTICE "ACPI is not supported!");
        return;
    }

    // try to find the signature of RSDP
    for (i = 0x000E0000; i < 0x000FFFFF; i++)
    {
        if (memcmp((char *)i, "RSD PTR ", 8) == 0)
        {
            printk(KERN_DEBUG "RSDP found at 0x%x", i);
            sdp = (struct XSDP *)i;
            break;
        }
    }

    // todo: also perform search in EBDA (https://wiki.osdev.org/RSDP)
    if (!sdp)
    {
        panic("Unable to find RSDP in memory!");
    }

    if (sdp->revision == 0)
    {
        printk(KERN_DEBUG "RSDP version is 1.0==");

        // RSDP checksum validation
        if (!(cpu_acpi_signature(sdp, sizeof(struct RSDP))))
            panic("ACPI checksum validation failed!");

        // parse RSDT
        root_sdt = (struct XSDT *)sdp->rsdt_address;
    }
    else if (sdp->revision == 2)
    {
        printk(KERN_DEBUG "RSDP version is 2.0>=");

        // XSDP checksum validation
        if (!(cpu_acpi_signature(sdp, sizeof(struct XSDP))))
            panic("ACPI XSDP checksum validation failed!");

// parse XSDT
#ifdef CONFIG_CPU_32BIT
        root_sdt = (struct XSDT *)(uint32_t)sdp->xsdt_address;
#else
        root_sdt = (struct XSDT *)sdp->xsdt_address;
#endif
    }

    // map the SDT
    kident(root_sdt, sizeof(struct XSDT), KERN_PAGE_RW);

    // R/XSDT checksum validation
    if (!(cpu_acpi_signature(root_sdt, root_sdt->header.length)))
        panic("ACPI RootSDT checksum validation failed!");

    acpi_loaded = true;
}

bool cpu_acpi_loaded()
{
    return acpi_loaded;
}

bool cpu_acpi_signature(void *ptr, size_t size)
{
    uint32_t checksum = 0, i;
    for (i = 0; i < size; i++)
        checksum += *(((char *)ptr) + i);
    return (checksum & 0xf) == 0;
}

struct XSDP *cpu_acpi_sdp()
{
    if (!sdp)
        panic("ACPI not initalized!");
    return sdp;
}

struct XSDT *cpu_acpi_root_sdt()
{
    if (!root_sdt)
        panic("ACPI not initalized!");
    return root_sdt;
}
bool cpu_acpi_shutdown()
{
    printk(KERN_WARNING "Shutdown ACPI feature is not implemented!");
    return false;
}

bool cpu_acpi_reboot()
{
    printk(KERN_WARNING "Reboot ACPI feature is not implemented!");
    return false;
}

uint32_t cpu_acpi_remap_irq(uint32_t irq)
{
    struct MADT_Entry *entry;
    uint8_t *i;

    // iterate thru all SDTs to find MADT
    ACPI_ITERATE(idx, pointer, {
        if (memcmp(pointer->signature, "APIC", 4) == 0)
        { // found MADT
            // iterate thru all MADT entries to get all APICs on the system
            struct MADT *madt = (struct MADT *)pointer;

            if (!madt || !cpu_acpi_signature(madt, madt->header.length))
                panic("Unable to find MADT record; or got invalid record (bad checksum)");
            // iterate thru all MADT entries to get all APICs on the system
            for (
                i = ((uint8_t *)(madt)) + 0x2c;
                i < ((uint8_t *)(madt)) + madt->header.length + 0x2c;)
            {
                entry = (struct MADT_Entry*)(i);

                if (entry->entry_type == APIC_IO_INT_SRC_OVERRIDE)
                {
                    struct IOAPIC_INTSRCO_MADT *int_overrd = (struct IOAPIC_INTSRCO_MADT *)(entry);

                    if (int_overrd->irq == irq)
                        return int_overrd->gsi;
                }

                i += entry->record_length;
            }
        }
    })

    return irq;
}

extern uintptr_t cpu_ioapic;

uint8_t cpu_acpi_load_madt(struct cpu_core *cores)
{
    kernel_assert(cores != NULL, "cores array is NULL");

    struct MADT_Entry *entry;
    size_t madts = 0;
    uint8_t physical_cpus = 0;
    uint8_t *i;

    // iterate thru all SDTs to find MADT
    ACPI_ITERATE(idx, pointer, {
        if (memcmp(pointer->signature, "APIC", 4) == 0)
        { // found MADT
            madts++;

            // iterate thru all MADT entries to get all APICs on the system
            struct MADT *madt = (struct MADT *)pointer;

            if (!madt || !cpu_acpi_signature(madt, madt->header.length))
                panic("Unable to find MADT record; or got invalid record (bad checksum)");

            // iterate thru all MADT entries to get all APICs on the system
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

                    cores[lapic_table->acpi_proc_id].lapic_id = lapic_table->apic_id;
                    cores[lapic_table->acpi_proc_id].lapic_ptr = madt->local_apic_address;

                    physical_cpus++;
                    printk(KERN_DEBUG "Local APIC found[%d]", lapic_table->acpi_proc_id);
                }
                else if (entry->entry_type == APIC_IO)
                {
                    struct IOAPIC_MADT *io_apic_table = (struct IOAPIC_MADT *)(entry);

                    // todo: only one IO/APIC can be used right now.
                    //       If another one was found, panic
                    if (cpu_ioapic)
                    {
                        printk(KERN_WARNING "Another IO/APIC was found, which is not supported (see the ACPI driver)");
                        goto next;
                    }

                    printk(KERN_DEBUG "IO/APIC found[%d]: 0x%x", io_apic_table->io_apic_id, io_apic_table->io_apic_address);
                    cpu_ioapic = io_apic_table->io_apic_address;
                }
                else if (entry->entry_type == APIC_IO_INT_SRC_OVERRIDE)
                {
                    struct IOAPIC_INTSRCO_MADT *int_overrd = (struct IOAPIC_INTSRCO_MADT *)(entry);

                    printk(KERN_DEBUG "IO/APIC ISO: bus = 0x%x, irq = 0x%x, gsi = 0x%x, flags = 0x%x",
                        int_overrd->bus, int_overrd->irq, int_overrd->gsi, int_overrd->flags);
                }
                
            next:
                i += entry->record_length;
            }
        }
    })

    return physical_cpus;
}
