#include <kernel/kernel.h>

#include <drivers/impl.h>
#include <drivers/cpu/cpu.h>
#include <drivers/cpu/interrupts/irq/irq.h>
#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/cpu/interrupts/idt/idt.h>
#include <drivers/cpu/interrupts/exceptions/exceptions.h>
#include <drivers/cpu/interrupts/pic/pic.h>
#include <drivers/cpu/timers/pit.h>
#include <drivers/cpu/timers/apic_timer.h>
#include <drivers/ps2/ps2.h>

void cpu_interrupts_init()
{
    __asm__ volatile("cli");
    extern uint8_t __cpu_count;

    cpu_idt_init();
    
    // initialize exceptions and IRQs
    cpu_int_excp_init();
    cpu_irq_init();
#ifdef CONFIG_CPU_NOAPIC
    printk(KERN_NOTICE "Using PIC only because CONFIG_CPU_NOAPIC option was set during compilation.");
    __cpu_count = 1;
    cpu_pic_init();
    cpu_pit_init();
#else
    // initialize APIC or PIC (APIC: edx APIC feature flag is set && ACPI is loaded ; Otherwise PIC)
    if (!(cpu_features_edx() & CPU_FEATURE_EDX_APIC) || !cpu_acpi_loaded())
    {
        printk(KERN_INFO "CPU does not support APIC. Using PIC instead (only one CPU core can be used).");

        __cpu_count = 1;
        cpu_pic_init();
        cpu_pit_init();
    }
    else
    {
        uintptr_t lapic, ioapic;
        __cpu_count = cpu_acpi_load_madt(&lapic, &ioapic);

        cpu_lapic_init(lapic);
        cpu_io_apic_init(ioapic);
        cpu_irq_apic_remap();

        cpu_pit_init();
        // cpu_atimer_init();
    }
#endif

    // initialize drivers
    ps2_init();

    // initialize interrupts
    __asm__ volatile("sti");
}

void cpu_interrupts_disable()
{
    // disable interrupts
    __asm__ volatile("cli");

    // Disable APIC/PIC
    if (cpu_using_apic())
    {
        cpu_io_apic_disable();
        cpu_lapic_disable();
    }
    else
        cpu_pic_disable();
}

int32_t __idt_subscribe(interrupt_handler_t ptr, uint8_t type);
void __idt_unsubscribe(uint8_t type, uint8_t id);

int32_t cpu_interrupts_subscribe(interrupt_handler_t handler, uint8_t int_no)
{
    return __idt_subscribe(handler, int_no);
}

void cpu_interrupts_unsubscribe(uint8_t int_no, int32_t id)
{
    if (id == -1)
        return;
    __idt_unsubscribe(int_no, id);
}

extern struct int_subscriber subscribed_interrupts[256][32];

uintptr_t __isr_handler(struct regs32 *regs)
{
    int32_t i;
    uintptr_t result = NULL;
    __asm__ volatile("cli");

    // send events to subscribed functions
    for (i = 0; i < 32; i++)
    {
        if (subscribed_interrupts[regs->int_no][i].in_use)
        {
            (subscribed_interrupts[regs->int_no][i].handler)(regs);
        }
    }

    if (regs->int_no < 32)
    { // exceptions
        result = cpu_int_excp_handle(regs);
    }
    else if (regs->int_no >= 32 && regs->int_no <= 47 && !cpu_using_apic())
    { // IRQs
        cpu_irq_acknowledge(regs->int_no - 32);
    }

    // write EOI if using APIC
    if (cpu_using_apic())
    {
        cpu_lapic_out(LAPIC_EOI, 0x0);
    }

end:
    __asm__ volatile("sti");
    return result;
}
