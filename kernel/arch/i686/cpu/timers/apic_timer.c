#include <kernel/kernel.h>
#include <kernel/driver/interrupt/interrupt.h>

#include <kernel/arch/i686/cpu/timers/apic_timer.h>
#include <kernel/arch/i686/cpu/interrupts/apic/local_apic.h>
#include <kernel/arch/i686/cpu/interrupts/apic/io_apic.h>
#include <kernel/arch/i686/cpu/interrupts/interrupts.h>
#include <kernel/arch/i686/cpu/acpi/acpi.h>
#include <kernel/arch/i686/cpu/cpu.h>
#include <kernel/arch/i686/impl.h>

void pit_early_prepare_sleep(size_t us);
void pit_early_sleep_disable();
void pit_early_sleep();

static void apic_timer_irq(struct cpu_core *core, struct regs32 *regs)
{
}

void cpu_atimer_init(struct cpu_core *core)
{
    // set divider
    cpu_lapic_out(core, LAPIC_TDCR, APIC_TIMER_DIV);

    // prepare sleep using PIT for 10ms to use it later
    pit_early_prepare_sleep(10000);

    // set initial counter for the APIC Timer (-1)
    cpu_lapic_out(core, LAPIC_TICR, 0xFFFFFFFF);

    // sleep for a bit, stop the timer and get elapsed ticks
    __enable_int();
    pit_early_sleep();
    __disable_int();
    cpu_lapic_out(core, LAPIC_TIMER, APIC_TIMER_DISABLE);

    core->apic_timer_ticks = (0xFFFFFFFF - cpu_lapic_in(core, LAPIC_TCCR));

    // disable PIT
    pit_early_sleep_disable();

    printk("apic_timer[%d]: 10ms elapsed ticks: %u", core->core_id, core->apic_timer_ticks);

    // start the timer
    kern_interrupts_install_handler(KERN_INTERRUPT_TIMER, &apic_timer_irq);
    cpu_lapic_out(core, LAPIC_TIMER, INTERRUPT_TIMER | APIC_TIMER_TMR_PERIODIC);
    cpu_lapic_out(core, LAPIC_TDCR, APIC_TIMER_DIV);
    cpu_lapic_out(core, LAPIC_TICR, (core->apic_timer_ticks * 1000) / INTERRUPT_TIMER_SPEED);
}

void cpu_atimer_disable(struct cpu_core *core)
{
    panic("apic_timer: disable is not implemented!");
}
