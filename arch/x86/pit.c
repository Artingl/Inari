#include <kernel/inari.h>
#include <kernel/timer.h>

#include <arch/x86/arch.h>
#include <arch/x86/pit.h>

#define PIT_OCW_MASK_BINCOUNT 1   // 00000001
#define PIT_OCW_MASK_MODE 0xE     // 00001110
#define PIT_OCW_MASK_RL 0x30      // 00110000
#define PIT_OCW_MASK_COUNTER 0xC0 // 11000000

#define PIT_OCW_BINCOUNT_BINARY 0 // 0
#define PIT_OCW_BINCOUNT_BCD 1    // 1

#define PIT_OCW_MODE_TERMINALCOUNT 0   // 0000
#define PIT_OCW_MODE_ONESHOT 0x2       // 0010
#define PIT_OCW_MODE_RATEGEN 0x4       // 0100
#define PIT_OCW_MODE_SQUAREWAVEGEN 0x6 // 0110
#define PIT_OCW_MODE_SOFTWARETRIG 0x8  // 1000
#define PIT_OCW_MODE_HARDWARETRIG 0xA  // 1010

#define PIT_OCW_RL_LATCH 0      // 000000
#define PIT_OCW_RL_LSBONLY 0x10 // 010000
#define PIT_OCW_RL_MSBONLY 0x20 // 100000
#define PIT_OCW_RL_DATA 0x30    // 110000

#define PIT_OCW_COUNTER_0 0    // 00000000
#define PIT_OCW_COUNTER_1 0x40 // 01000000
#define PIT_OCW_COUNTER_2 0x80 // 10000000

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND  0x43

#define PIT_DIVIDER      250
#define PIT_FREQUENCY_HZ 1193182

int x86_pit_init()
{
    /* Setup the pit */
    uint8_t ocw = 0;
    ocw = (ocw & ~PIT_OCW_MASK_MODE) | PIT_OCW_MODE_SQUAREWAVEGEN;
    ocw = (ocw & ~PIT_OCW_MASK_RL) | PIT_OCW_RL_DATA;
    ocw = (ocw & ~PIT_OCW_MASK_COUNTER) | PIT_OCW_COUNTER_0;
    x86_outb(PIT_COMMAND, ocw);

    /* Set the pit divider */
	x86_outb(PIT_CHANNEL0, PIT_DIVIDER & 0xFF);
	x86_outb(PIT_CHANNEL0, (PIT_DIVIDER >> 8) & 0xff);

    timer_init(PIT_FREQUENCY_HZ / PIT_DIVIDER);
    return 0;
}

void x86_pit_irq()
{
    timer_tick();
}