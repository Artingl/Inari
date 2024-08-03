#include <kernel/kernel.h>
#include <kernel/include/C/string.h>

#include <kernel/arch/i386/impl.h>
#include <kernel/arch/i386/cpu/cpu.h>
#include <kernel/arch/i386/cpu/acpi/acpi.h>
#include <kernel/arch/i386/cpu/timers/pit.h>
#include <kernel/arch/i386/cpu/timers/apic_timer.h>
#include <kernel/arch/i386/cpu/interrupts/interrupts.h>
#include <kernel/arch/i386/cpu/interrupts/exceptions/exceptions.h>
#include <kernel/arch/i386/cpu/interrupts/pic/pic.h>
#include <kernel/arch/i386/cpu/interrupts/irq/irq.h>
#include <kernel/arch/i386/cpu/interrupts/apic/local_apic.h>
#include <kernel/arch/i386/cpu/interrupts/apic/io_apic.h>

void cpu_ints_core_init(struct cpu_core *core)
{
    cpu_idt_init(core);
    
    // initialize exceptions and IRQs
    cpu_exceptions_core_init(core);
    cpu_irq_init(core);
#ifdef CONFIG_CPU_NOAPIC
    printk("Using PIC only because CONFIG_CPU_NOAPIC option was set during compilation.");
    cpu_pic_init();
    cpu_pit_init();
#else
    // initialize APIC or PIC (APIC: edx APIC feature flag is set && ACPI is loaded ; Otherwise PIC)
    if (!(cpu_feat_edx() & CPU_FEATURE_EDX_APIC) || !cpu_acpi_loaded())
    {
        printk("CPU does not support APIC. Using PIC instead (only one CPU core can be used).");
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

struct {
    int occupied;

    int int_no;
    interrupt_handler_t handler;

} interrupts_subs[256];

void cpu_ints_init()
{
    memset((void*)&interrupts_subs, 0, sizeof(interrupts_subs));
}

void cpu_ints_sub(int int_no, interrupt_handler_t handler)
{
    size_t i;
    for (i = 0; i < 256; i++)
    {
        if (!interrupts_subs[i].occupied || interrupts_subs[i].handler == handler)
        {
            interrupts_subs[i].occupied = 1;
            interrupts_subs[i].int_no = int_no;
            interrupts_subs[i].handler = handler;
            return;
        }
    }

    panic("TODO: No free slots for interrupt handlers were found!");
}

void cpu_ints_unsub(interrupt_handler_t handler)
{
    size_t i;
    for (i = 0; i < 256; i++)
    {
        if (interrupts_subs[i].handler == handler)
        {
            interrupts_subs[i].occupied = 0;
        }
    }
}

uintptr_t isr_handler(struct regs32 *regs)
{
    uint32_t i;
    void *ret;
    struct cpu_core *core = cpu_current_core();

    if (!core->is_bsp)
        printk("isr_handler: !!! %d", regs->int_no);

    // send events to subscribed functions
    for (i = 0; i < 256; i++)
    {
        if (interrupts_subs[i].occupied && regs->int_no == interrupts_subs[i].int_no && interrupts_subs[i].handler != NULL)
        {
            interrupts_subs[i].handler(core, regs);
        }
    }

    if (regs->int_no < 32)
    { // exceptions
        cpu_exceptions_core_handle(core, regs);
    }
    else if (regs->int_no >= 32 && regs->int_no <= 47 && !cpu_using_apic())
    { // IRQs
        cpu_irq_acknowledge(regs->int_no - 32);
    }

    // write EOI if using APIC
    if (cpu_using_apic())
    {
        cpu_lapic_out(core, LAPIC_EOI, 0x0);
    }

    return 0;
}
