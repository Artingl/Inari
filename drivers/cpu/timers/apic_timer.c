#include <kernel/kernel.h>

#include <drivers/cpu/timers/apic_timer.h>
#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/cpu.h>
#include <drivers/impl.h>

interrupt_handler_t apic_timer_irq(struct cpu_core *core, struct regs32 *regs);

void pit_early_prepare_sleep(size_t us);
void pit_early_sleep_disable();
void pit_early_sleep();

extern double cpu_timer_ticks;

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

    core->apic_timer_ticks = 0xFFFFFFFF - cpu_lapic_in(core, LAPIC_TCCR);

    // disable PIT
    pit_early_sleep_disable();

    printk(KERN_DEBUG "apic_timer#%d: 10ms elapsed ticks: %u", core->core_id, core->apic_timer_ticks);

    // start the timer
    cpu_ints_sub(INTERRUPT_TIMER, &apic_timer_irq);
    cpu_lapic_out(core, LAPIC_TIMER, INTERRUPT_TIMER | APIC_TIMER_TMR_PERIODIC);
    cpu_lapic_out(core, LAPIC_TDCR, APIC_TIMER_DIV);
    cpu_lapic_out(core, LAPIC_TICR, (core->apic_timer_ticks * 1000) / INTERRUPT_TIMER_SPEED);
}

void cpu_atimer_disable(struct cpu_core *core)
{
    // todo
}

interrupt_handler_t apic_timer_irq(struct cpu_core *core, struct regs32 *regs)
{
    if (core->is_bsp)
        cpu_timer_ticks++;
}
