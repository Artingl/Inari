#include <kernel/kernel.h>
#include <kernel/driver/memory/memory.h>
#include <kernel/libc/typedefs.h>
#include <kernel/libc/string.h>

#include <kernel/arch/i686/impl.h>
#include <kernel/arch/i686/cpu/acpi/acpi.h>
#include <kernel/arch/i686/cpu/cpu.h>
#include <kernel/arch/i686/memory/vmm.h>
#include <kernel/arch/i686/memory/memory.h>

struct XSDP *sdp = NULL;
struct XSDT *root_sdt = NULL;

int acpi_loaded = false;

static int cpu_acpi_signature(void *ptr, size_t size)
{
    uint32_t checksum = 0, i;
    for (i = 0; i < size; i++)
        checksum += *(((char *)ptr) + i);
    return (checksum & 0xf) == 0;
}

int cpu_acpi_init()
{
    size_t i;

    if (!(cpu_feat_edx() & CPU_FEATURE_EDX_ACPI))
    {
        printk("acpi: is not supported!");
        return 1;
    }

    // try to find the signature of RSDP
    for (i = 0x000E0000; i < 0x000FFFFF; i+=8)
    {
        if (memcmp((char *)i, "RSD PTR ", 8) == 0)
        {
            sdp = (struct XSDP *)i;
            printk("acpi: RSDP found at 0x%x", i);
            break;
        }
    }

    // todo: also perform search in EBDA (https://wiki.osdev.org/RSDP)
    if (!sdp)
    {
        printk("acpi: unable to find RSDP in memory!");
        return 1;
    }

    if (sdp->revision == 0)
    {
        printk("acpi: SDP version 1.0==");

        // RSDP checksum validation
        if (!(cpu_acpi_signature(sdp, sizeof(struct RSDP))))
            panic("acpi: RSDP checksum validation failed!");

        // parse RSDT
        root_sdt = (struct XSDT *)sdp->rsdt_address;
    }
    else if (sdp->revision == 2)
    {
        printk("acpi: SDP version 2.0>=");

        // XSDP checksum validation
        if (!(cpu_acpi_signature(sdp, sizeof(struct XSDP))))
            panic("acpi: XSDP checksum validation failed!");

        // parse XSDT
        root_sdt = (struct XSDT *)(uint32_t)sdp->xsdt_address;
        if (!root_sdt)
            root_sdt = (struct XSDT *)sdp->rsdt_address;
    }
    else {
        printk("acpi: invalid SDP revision");
        return 1;
    }

    // map the SDT
    kident(root_sdt - PAGE_SIZE * 128, PAGE_SIZE * 128, KERN_PAGE_RW);
    kident(root_sdt, PAGE_SIZE * 128, KERN_PAGE_RW);

    if (root_sdt->header.length == 0)
    {
        printk("acpi: r/xSDT header length is 0, base=0x%x", root_sdt);
        return 1;
    }

    acpi_loaded = true;
    memory_forbid_region((uintptr_t)sdp, PAGE_SIZE);
    memory_forbid_region((uintptr_t)root_sdt, PAGE_SIZE);
    ACPI_ITERATE(idx, pointer, {
        memory_forbid_region((uintptr_t)pointer, PAGE_SIZE);
    })

    // R/XSDT checksum validation
    if (!(cpu_acpi_signature(root_sdt, root_sdt->header.length)))
        panic("acpi: RootSDT checksum validation failed!");

    return 0;
}

int cpu_acpi_loaded()
{
    return acpi_loaded;
}

struct XSDP *cpu_acpi_sdp()
{
    if (!cpu_acpi_loaded())
        panic("acpi: not initalized");
    return sdp;
}

struct XSDT *cpu_acpi_root_sdt()
{
    if (!cpu_acpi_loaded())
        panic("acpi: not initalized");
    return root_sdt;
}
int cpu_acpi_poweroff()
{
    printk(KERN_WARNING "acpi: shutdown ACPI feature is not implemented!");
    return false;
}

int cpu_acpi_reboot()
{
    if (!cpu_acpi_loaded())
        panic("acpi: not initalized");
    
    // Call reset command
    ACPI_ITERATE(idx, pointer, {
        if (memcmp(pointer->signature, "FACP", 4) == 0)
        {
            struct FADT *fadt = (struct FADT *)pointer;

            // try to output a byte
            __outb(fadt->reset_reg.address, fadt->reset_value);

            // try to make mmap call
            uint8_t *reg = (uint8_t *)((uintptr_t)fadt->reset_reg.address);
            *reg = fadt->reset_value;
        }
    })

    printk(KERN_WARNING "acpi: unable to call ACPI reset command");
    return false;
}

uint32_t cpu_acpi_remap_irq(uint32_t irq)
{
    if (!cpu_acpi_loaded())
        panic("acpi: not initalized");
    
    struct MADT_Entry *entry;
    uint8_t *i;

    // iterate thru all SDTs to find MADT
    ACPI_ITERATE(idx, pointer, {
        if (memcmp(pointer->signature, "APIC", 4) == 0)
        { // found MADT
            // iterate thru all MADT entries to get all APICs on the system
            struct MADT *madt = (struct MADT *)pointer;

            if (!madt || !cpu_acpi_signature(madt, madt->header.length))
                panic("acpi: unable to find MADT record; or got invalid record (bad checksum)");
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
    if (!cpu_acpi_loaded())
        panic("acpi: not initalized");
    
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
                panic("acpi: unable to find MADT record; or got invalid record (bad checksum)");

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
                    printk("acpi: lapic found[%d]", lapic_table->acpi_proc_id);
                }
                else if (entry->entry_type == APIC_IO)
                {
                    struct IOAPIC_MADT *io_apic_table = (struct IOAPIC_MADT *)(entry);

                    // todo: only one IO/APIC can be used right now.
                    //       If another one was found, panic
                    if (cpu_ioapic)
                    {
                        printk(KERN_WARNING "acpi: another io/apic was found, which is not supported (see the ACPI driver)");
                        goto next;
                    }

                    printk("acpi: io/apic found[%d]: 0x%x", io_apic_table->io_apic_id, io_apic_table->io_apic_address);
                    cpu_ioapic = io_apic_table->io_apic_address;
                }
                else if (entry->entry_type == APIC_IO_INT_SRC_OVERRIDE)
                {
                    struct IOAPIC_INTSRCO_MADT *int_overrd = (struct IOAPIC_INTSRCO_MADT *)(entry);

                    printk("acpi: io/apic ISO: bus = 0x%x, irq = 0x%x, gsi = 0x%x, flags = 0x%x",
                        int_overrd->bus, int_overrd->irq, int_overrd->gsi, int_overrd->flags);
                }
                
            next:
                i += entry->record_length;
            }
        }
    })

    return physical_cpus;
}
