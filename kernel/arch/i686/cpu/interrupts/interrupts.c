#include <kernel/kernel.h>
#include <kernel/include/C/string.h>
#include <kernel/list/dynlist.h>
#include <kernel/driver/interrupt/interrupt.h>

#include <kernel/arch/i686/impl.h>
#include <kernel/arch/i686/cpu/cpu.h>
#include <kernel/arch/i686/cpu/acpi/acpi.h>
#include <kernel/arch/i686/cpu/timers/pit.h>
#include <kernel/arch/i686/cpu/timers/apic_timer.h>
#include <kernel/arch/i686/cpu/interrupts/interrupts.h>
#include <kernel/arch/i686/cpu/interrupts/exceptions/exceptions.h>
#include <kernel/arch/i686/cpu/interrupts/pic/pic.h>
#include <kernel/arch/i686/cpu/interrupts/irq/irq.h>
#include <kernel/arch/i686/cpu/interrupts/apic/local_apic.h>
#include <kernel/arch/i686/cpu/interrupts/apic/io_apic.h>

void cpu_ints_core_init(struct cpu_core *core)
{
    cpu_idt_init(core);
    
    // initialize exceptions and IRQs
    cpu_exceptions_core_init(core);
    cpu_irq_init(core);
#ifdef CONFIG_I686_NOAPIC
    printk("ints: using PIC only because CONFIG_I686_NOAPIC option was set during compilation.");
    cpu_pic_init();
    cpu_pit_init();
#else
    // initialize APIC or PIC (APIC: edx APIC feature flag is set && ACPI is loaded ; Otherwise PIC)
    if (!(cpu_feat_edx() & CPU_FEATURE_EDX_APIC) || !cpu_acpi_loaded())
    {
        printk("ints: CPU does not support APIC, or we had an error initializing it. Using PIC instead.");
        cpu_pic_init();
        cpu_pit_init();
    }
    else
    {
        cpu_lapic_init(core);
        if (core->is_bsp) {
            cpu_atimer_init(core);
            cpu_pit_disable();
        }
    }
#endif

    core->ints_loaded = 1;
}

void cpu_ints_core_disable(struct cpu_core *core)
{
    core->ints_loaded = 0;
    // Disable APIC/PIC
    if (cpu_using_apic())
        cpu_lapic_disable(core);
    else
        cpu_pic_disable();
}

extern void kern_interrupts_arch_handle(uint8_t int_no);

uintptr_t isr_handler(struct regs32 *regs)
{
    uint32_t i;
    void *ret;
    struct cpu_core *core = cpu_current_core();

    // Forward the interrupt to the kernel's interrupt handler using the IDs that the kernel understands
    if (regs->int_no == INTERRUPT_TIMER)
        kern_interrupts_arch_handle(KERN_INTERRUPT_TIMER);
    else if (regs->int_no == INTERRUPT_PS2)
        kern_interrupts_arch_handle(KERN_INTERRUPT_PS2);
    else
    {
        // If we don't know the source and/or ID of the interrupt, send it as it is
        kern_interrupts_arch_handle(regs->int_no);
    }

    if (regs->int_no < 32)
    {
        // Handle exceptions
        cpu_exceptions_core_handle(core, regs);
    }
    else if (regs->int_no >= 32 && regs->int_no <= 47 && !cpu_using_apic())
    {
        // Acknowledge the IRQ
        cpu_irq_acknowledge(regs->int_no - 32);
    }

    // write EOI if using APIC
    if (cpu_using_apic())
    {
        cpu_lapic_out(core, LAPIC_EOI, 0x0);
    }

    return 0;
}