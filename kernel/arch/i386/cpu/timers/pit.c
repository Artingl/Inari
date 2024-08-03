#include <kernel/kernel.h>

#include <kernel/arch/i386/cpu/cpu.h>
#include <kernel/arch/i386/cpu/timers/pit.h>
#include <kernel/arch/i386/cpu/interrupts/interrupts.h>
#include <kernel/arch/i386/impl.h>

void pit_irq(struct cpu_core *core, struct regs32 *regs);

volatile double divider;
volatile double irq_ticks;
extern double cpu_timer_ticks;

static inline void pit_send_cmd(uint8_t cmd)
{
    __outb(PIT_CM, cmd);
}

static inline void pit_send_data(uint16_t data, uint8_t counter)
{
    uint8_t port = (counter == PIT_OCW_COUNTER_0) ? PIT_DATA0 : ((counter == PIT_OCW_COUNTER_1) ? PIT_DATA1 : PIT_DATA2);
    __outb(port, (uint8_t)data);
}

static inline void pit_set_div(uint16_t div)
{
    divider = (double)div;
    uint16_t divisor = (uint16_t)(1193181 / (uint16_t)div);

    // send operational command words
    uint8_t ocw = 0;
    ocw = (ocw & ~PIT_OCW_MASK_MODE) | PIT_OCW_MODE_SQUAREWAVEGEN;
    ocw = (ocw & ~PIT_OCW_MASK_RL) | PIT_OCW_RL_DATA;
    ocw = (ocw & ~PIT_OCW_MASK_COUNTER) | PIT_OCW_COUNTER_0;
    pit_send_cmd(ocw);

    // set frequency rate
    pit_send_data(divisor & 0xff, 0);
    pit_send_data((divisor >> 8) & 0xff, 0);
}

void cpu_pit_init()
{
    cpu_timer_ticks = 0;
    irq_ticks = 0;
    cpu_ints_sub(INTERRUPT_TIMER, &pit_irq);
    pit_set_div(INTERRUPT_TIMER_SPEED);
}

void cpu_pit_disable()
{
    // set pit to one-shot mode
    uint8_t ocw = 0;
    ocw = (ocw & ~PIT_OCW_MASK_MODE) | PIT_OCW_MODE_ONESHOT;
    ocw = (ocw & ~PIT_OCW_MASK_RL) | PIT_OCW_RL_DATA;
    pit_send_cmd(ocw);
    cpu_ints_unsub(&pit_irq);
}

uint32_t cpu_pit_read()
{
    uint32_t count = 0;

    // al = channel in bits 6 and 7, remaining bits clear
    __outb(PIT_CM, 0b0000000);

    count = __inb(PIT_DATA0);       // Low byte
    count |= __inb(PIT_DATA0) << 8; // High byte
    return count;
}

void pit_irq(struct cpu_core *core, struct regs32 *regs)
{
    if (!core->is_bsp)
        cpu_timer_ticks++;
    irq_ticks++;
}

// we need to tell the compiler not to optimize these functions, because then it will mess up some of them
volatile double pit_early_us;

void __attribute__((optimize("O0"))) pit_early_prepare_sleep(size_t us)
{
    irq_ticks = 0;
    pit_early_us = ((double)us) / 1000000.0f * INTERRUPT_TIMER_SPEED;

    cpu_ints_sub(INTERRUPT_TIMER, &pit_irq);
    pit_set_div(INTERRUPT_TIMER_SPEED);
}

void __attribute__((optimize("O0"))) pit_early_sleep_disable()
{
    cpu_pit_disable();
}

void __attribute__((optimize("O0"))) pit_early_sleep()
{
    volatile double start = cpu_timer_ticks;
    
    while (start + pit_early_us > irq_ticks)
    {}
}