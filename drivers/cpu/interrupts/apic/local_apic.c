#include <kernel/kernel.h>

#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/interrupts/idt/idt.h>
#include <drivers/cpu/interrupts/pic/pic.h>
#include <drivers/cpu/interrupts/irq/irq.h>

#include <drivers/impl.h>

#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/string.h>

uintptr_t __lapic;

void cpu_lapic_init(uintptr_t lapic)
{
    // map lapic memory
    // kmmap(LAPIC_BASE, lapic, PAGE_SIZE * 2, KERN_PAGE_RW);
    kident(lapic - PAGE_SIZE, PAGE_SIZE * 2, KERN_PAGE_RW);
    __lapic = lapic;

    // enable all kinds of interrupts
    cpu_lapic_out(LAPIC_TPR, 0);

    // enable and disable PIC so it will do all stuff we need
    cpu_pic_apic_config();

    // initialize the Local APIC
    cpu_lapic_set_base(cpu_lapic_get_base());
    cpu_lapic_out(LAPIC_SVR, 0x100 | 0xff);

    printk(KERN_INFO "LAPIC address: %p", (unsigned long)cpu_lapic_get_base());
}

uintptr_t cpu_lapic_get_base()
{
    // uint32_t eax, edx;
    // __get_msr(IA32_APIC_BASE, &eax, &edx);

    // return (eax & 0xfffff000);

    return __lapic;
}

void cpu_lapic_set_base(uintptr_t base)
{
    uint32_t eax = (base & 0xfffff0000) | IA32_APIC_BASE_ENABLE, edx;
    __set_msr(IA32_APIC_BASE, eax, edx);
}

uint32_t cpu_lapic_in(uint32_t addr)
{
    return *((volatile uint32_t*)(cpu_lapic_get_base() + addr));
}

void cpu_lapic_out(uint32_t addr, uint32_t value)
{
    *((volatile uint32_t*)(cpu_lapic_get_base() + addr)) = value;
}

void cpu_lapic_disable()
{
    // todo...
}
