#include <kernel/kernel.h>

#include <drivers/cpu/timers/apic_timer.h>
#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/cpu.h>
#include <drivers/impl.h>

interrupt_handler_t __apic_timer_irq(struct regs32 *regs);

void pit_early_prepare_sleep(size_t us);
void pit_early_sleep_disable();
void pit_early_sleep();

extern double cpu_timer_ticks;

void cpu_atimer_init(struct cpu_core *core)
{
    uint32_t elapsed_ticks;

    // set divider
    cpu_lapic_out(core, LAPIC_TDCR, APIC_TIMER_DIV);

    // prepare sleep using PIT for 10ms to use it later
    pit_early_prepare_sleep(10000);

    // set initial counter for the APIC Timer (-1)
    cpu_lapic_out(core, LAPIC_TICR, 0xFFFFFFFF);

    // sleep for a bit, stop the timer and get elapsed ticks
    pit_early_sleep();
    cpu_lapic_out(core, LAPIC_TIMER, APIC_TIMER_DISABLE);

    elapsed_ticks = 0xFFFFFFFF - cpu_lapic_in(core, LAPIC_TCCR);

    // disable PIT
    pit_early_sleep_disable();

    // start the time on IRQ 0 (32nd interrupt), set divider
    cpu_lapic_out(core, LAPIC_TIMER, INTERRUPT_TIMER | APIC_TIMER_TMR_PERIODIC);
    cpu_lapic_out(core, LAPIC_TDCR, APIC_TIMER_DIV);
    cpu_lapic_out(core, LAPIC_TICR, (elapsed_ticks * 100) / INTERRUPT_TIMER_SPEED);

    // cpu_ints_subscribe_core(core, &__apic_timer_irq, INTERRUPT_TIMER);
    printk(KERN_DEBUG "APIC Timer elapsed ticks in 10ms: %u", elapsed_ticks);

}

void cpu_atimer_disable(struct cpu_core *core)
{
    // todo
}

interrupt_handler_t __apic_timer_irq(struct regs32 *regs)
{
    cpu_timer_ticks++;
}
