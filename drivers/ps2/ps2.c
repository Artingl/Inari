#include <kernel/kernel.h>

#include <kernel/arch/i386/impl.h>

#include <drivers/ps2/ps2.h>
#include <kernel/arch/i386/cpu/interrupts/interrupts.h>

void ps2_handler(struct cpu_core *core, struct regs32 *regs);

void ps2_init()
{
    // uncomment line in the IRQ APIC remap
    printk("ps2: subscribing IRQs");
    cpu_ints_sub(INTERRUPT_PS2, &ps2_handler);
}

void ps2_handler(struct cpu_core *core, struct regs32 *regs)
{
    uint8_t c = __inb(PS2_PORT);

    // send ACK to the keyboard
    // __outb(PS2_PORT, 0xFA);
}
