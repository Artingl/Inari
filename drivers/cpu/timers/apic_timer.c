#include <kernel/kernel.h>

#include <drivers/cpu/timers/apic_timer.h>
#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/cpu.h>
#include <drivers/impl.h>

interrupt_handler_t __apic_timer_irq(struct regs32 *regs);

void __pit_early_prepare_sleep(size_t us);
void __pit_early_sleep_disable();
void __pit_early_sleep();

extern double __timer_ticks;

void cpu_atimer_init()
{
    uint32_t elapsed_ticks;

    // set divider
    cpu_lapic_out(LAPIC_TDCR, APIC_TIMER_DIV);

    // prepare sleep using PIT for 10ms to use it later
    __pit_early_prepare_sleep(10000);

    // set initial counter for the APIC Timer (-1)
    cpu_lapic_out(LAPIC_TICR, 0xFFFFFFFF);

    // sleep for a bit, stop the timer and get elapsed ticks
    __pit_early_sleep();
    cpu_lapic_out(LAPIC_TIMER, APIC_TIMER_DISABLE);

    elapsed_ticks = 0xFFFFFFFF - cpu_lapic_in(LAPIC_TCCR);

    // disable PIT
    __pit_early_sleep_disable();

    // start the time on IRQ 0 (32nd interrupt), set divider
    cpu_lapic_out(LAPIC_TIMER, INTERRUPT_TIMER | APIC_TIMER_TMR_PERIODIC);
    cpu_lapic_out(LAPIC_TDCR, APIC_TIMER_DIV);
    cpu_lapic_out(LAPIC_TICR, (elapsed_ticks * 100) / INTERRUPT_TIMER_SPEED);

    cpu_interrupts_subscribe(&__apic_timer_irq, INTERRUPT_TIMER);
    printk(KERN_DEBUG "APIC Timer elapsed ticks in 10ms: %u", elapsed_ticks);

}

void cpu_atimer_disable()
{
    // todo
}

interrupt_handler_t __apic_timer_irq(struct regs32 *regs)
{
    __timer_ticks++;
}
