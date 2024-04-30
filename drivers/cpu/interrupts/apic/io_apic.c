#include <kernel/kernel.h>

#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/memory/memory.h>
#include <drivers/cpu/interrupts/pic/pic.h>
#include <drivers/cpu/interrupts/irq/irq.h>

#include <drivers/impl.h>

#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/string.h>

uintptr_t __ioapic;

void cpu_io_apic_init(uintptr_t ioapic)
{
    memory_forbid_region(ioapic, PAGE_SIZE * 4);

    // map io apic memory
    // kmmap(IO_APIC_BASE, ioapic, PAGE_SIZE * 2, KERN_PAGE_RW);
    kident((void*)(ioapic - PAGE_SIZE), PAGE_SIZE * 2, KERN_PAGE_RW);
    __ioapic = ioapic;

    size_t i;
    uint32_t count = ((cpu_io_apic_read_reg(IOAPICVER) >> 16) & 0xff) + 1;

    printk("ioapic: address: %p", (unsigned long)cpu_io_apic_get_base());
    printk("ioapic: max IRQs to handle: %d", count);

    for (i = 0; i < count; i++)
        cpu_io_apic_map(i, 1 << 16);
}

void cpu_io_apic_disable()
{
}

void cpu_io_apic_write_reg(uint8_t reg, uint32_t value)
{
    *((volatile uint32_t*)(cpu_io_apic_get_base() + IOREGSEL)) = reg;
    *((volatile uint32_t*)(cpu_io_apic_get_base() + IOREGWIN)) = value;
}

uint32_t cpu_io_apic_read_reg(uint8_t reg)
{
    *((volatile uint32_t*)(cpu_io_apic_get_base() + IOREGSEL)) = reg;
    return *((volatile uint32_t*)(cpu_io_apic_get_base() + IOREGWIN));
}

void cpu_io_apic_map(uint8_t index, uint64_t value)
{
    cpu_io_apic_write_reg(IOAPICREDTBL(index), (uint32_t)(value));
    cpu_io_apic_write_reg(IOAPICREDTBL(index) + 1, (uint32_t)(value >> 32));
}

uintptr_t cpu_io_apic_get_base()
{
    return __ioapic;
}
