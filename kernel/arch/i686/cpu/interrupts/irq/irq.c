#include <kernel/kernel.h>
#include <kernel/include/string.h>

#include <kernel/arch/i686/cpu/interrupts/interrupts.h>
#include <kernel/arch/i686/cpu/interrupts/pic/pic.h>
#include <kernel/arch/i686/cpu/acpi/acpi.h>
#include <kernel/arch/i686/cpu/interrupts/apic/io_apic.h>
#include <kernel/arch/i686/cpu/interrupts/apic/local_apic.h>
#include <kernel/arch/i686/cpu/interrupts/irq/irq.h>

size_t spurious_interrupts = 0;

extern void _irq0();
extern void _irq1();
extern void _irq2();
extern void _irq3();
extern void _irq4();
extern void _irq5();
extern void _irq6();
extern void _irq7();
extern void _irq8();
extern void _irq9();
extern void _irq10();
extern void _irq11();
extern void _irq12();
extern void _irq13();
extern void _irq14();
extern void _irq15();

void cpu_irq_init(struct cpu_core *core)
{
    cpu_interrupts_idt_install(core, (unsigned)_irq0, 32, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq1, 33, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq2, 34, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq3, 35, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq4, 36, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq5, 37, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq6, 38, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq7, 39, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq8, 40, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq9, 41, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq10, 42, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq11, 43, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq12, 44, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq13, 45, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq14, 46, 0x08, 0x8e);
    cpu_interrupts_idt_install(core, (unsigned)_irq15, 47, 0x08, 0x8e);

    spurious_interrupts = 0;
}

void cpu_irq_apic_remap()
{
    // the IRQ_PIT value will also work for the APIC Timer
    cpu_io_apic_map(cpu_acpi_remap_irq(IRQ_PIT), 0x20 + IRQ_PIT);
    cpu_io_apic_map(cpu_acpi_remap_irq(IRQ_PS2), 0x20 + IRQ_PS2);
}

size_t cpu_irq_spurious_count()
{
    return spurious_interrupts;
}

void cpu_irq_acknowledge(uint8_t irq_no)
{
    if (irq_no == 7)
    {
        spurious_interrupts++;
        return;
    }

    if (irq_no >= 12)
        __outb(PIC2_COMMAND, 0x20);
    __outb(PIC1_COMMAND, 0x20);
}

void cpu_irq_mask(unsigned char irq_line)
{
    uint16_t port;
    uint8_t value;

    if (irq_line < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        irq_line -= 8;
    }

    value = __inb(port) | (1 << irq_line);
    __outb(port, value);
}