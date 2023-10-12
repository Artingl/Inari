#include <kernel/kernel.h>

#include <drivers/cpu/timers/pit.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/impl.h>

interrupt_handler_t __pit_irq(struct regs32 *regs);

int32_t int_id = -1;
double divider;

extern double __timer_ticks;

static inline void __pit_send_cmd(uint8_t cmd)
{
    __outb(PIT_CM, cmd);
}

static inline void __pit_send_data(uint16_t data, uint8_t counter)
{
    uint8_t port = (counter == PIT_OCW_COUNTER_0) ? PIT_DATA0 : ((counter == PIT_OCW_COUNTER_1) ? PIT_DATA1 : PIT_DATA2);

    __outb(port, (uint8_t)data);
}

static inline void __pit_set_div(uint16_t div)
{
    divider = (double)div;
    uint16_t divisor = (uint16_t)(1193181 / (uint16_t)div);

    // send operational command words
    uint8_t ocw = 0;
    ocw = (ocw & ~PIT_OCW_MASK_MODE) | PIT_OCW_MODE_SQUAREWAVEGEN;
    ocw = (ocw & ~PIT_OCW_MASK_RL) | PIT_OCW_RL_DATA;
    ocw = (ocw & ~PIT_OCW_MASK_COUNTER) | PIT_OCW_COUNTER_0;
    __pit_send_cmd(ocw);

    // set frequency rate
    __pit_send_data(divisor & 0xff, 0);
    __pit_send_data((divisor >> 8) & 0xff, 0);
}

void cpu_pit_init()
{
    if (int_id != -1)
    {
        cpu_interrupts_unsubscribe(INTERRUPT_TIMER, int_id);
    }

    __timer_ticks = 0;
    int_id = cpu_interrupts_subscribe(&__pit_irq, INTERRUPT_TIMER);
    __pit_set_div(INTERRUPT_TIMER_SPEED);
}

void cpu_pit_disable()
{
    // set pit to one-shot mode
    uint8_t ocw = 0;
    ocw = (ocw & ~PIT_OCW_MASK_MODE) | PIT_OCW_MODE_ONESHOT;
    ocw = (ocw & ~PIT_OCW_MASK_RL) | PIT_OCW_RL_DATA;
    __pit_send_cmd(ocw);
    cpu_interrupts_unsubscribe(INTERRUPT_TIMER, int_id);
    int_id = -1;
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

interrupt_handler_t __pit_irq(struct regs32 *regs)
{
    __timer_ticks++;
}

// we need to tell the compiler not to optimize these functions, because then it will mess up some of them
double __pit_prepare_us;
bool __pit_prepare_interrupts;

void __attribute__((optimize("O0"))) __pit_early_prepare_sleep(size_t us)
{
    __timer_ticks = 0;
    __pit_prepare_interrupts = !__eint();
    __pit_prepare_us = ((double)us) / 1000000.0f * INTERRUPT_TIMER_SPEED;

    if (int_id != -1)
    {
        cpu_interrupts_unsubscribe(INTERRUPT_TIMER, int_id);
    }

    int_id = cpu_interrupts_subscribe(&__pit_irq, INTERRUPT_TIMER);
    __pit_set_div(INTERRUPT_TIMER_SPEED);
}

void __attribute__((optimize("O0"))) __pit_early_sleep_disable()
{
    if (__pit_prepare_interrupts)
        __asm__ volatile("cli");

    cpu_pit_disable();
    __pit_prepare_interrupts = false;
}

void __attribute__((optimize("O0"))) __pit_early_sleep()
{
    if (__pit_prepare_interrupts)
        __asm__ volatile("sti");
    double start = __timer_ticks;
    
    while (start + __pit_prepare_us > __timer_ticks)
    {}
}