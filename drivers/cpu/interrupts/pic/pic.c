#include <kernel/kernel.h>

#include <drivers/cpu/interrupts/pic/pic.h>
#include <drivers/cpu/interrupts/irq/irq.h>
#include <drivers/cpu/timers/pit.h>
#include <drivers/cpu/interrupts/exceptions/exceptions.h>
#include <drivers/cpu/cpu.h>
#include <drivers/impl.h>

bool __pic_enabled = false;

void cpu_pic_init()
{
    // reset PICs
    __outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    __io_wait();
    __outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    __io_wait();

    // set PIC offsets
    __outb(PIC1_DATA, PIC1_OFFSET);
    __io_wait();
    __outb(PIC2_DATA, PIC2_OFFSET);
    __io_wait();

    // tell PICs whose of them is slave and master
    __outb(PIC1_DATA, 4);
    __io_wait();
    __outb(PIC2_DATA, 2);
    __io_wait();

    // tell them to be in 8086 mode
    __outb(PIC1_DATA, ICW4_8086);
    __io_wait();
    __outb(PIC2_DATA, ICW4_8086);
    __io_wait();

    // set masks
    __outb(PIC1_DATA, 0x00);
    __io_wait();
    __outb(PIC2_DATA, 0x00);
    __io_wait();

    __pic_enabled = true;
}

// configure PIC for the APIC
void cpu_pic_apic_config()
{
    cpu_pic_init();
    cpu_pic_disable();
}

bool cpu_pic_is_enabled()
{
    return __pic_enabled;
}

void cpu_pic_disable()
{
    size_t i;
    __pic_enabled = false;

    // mask IRQs
    for (i = 0; i < 15; ++i)
        cpu_irq_mask(i);

    // disable PIC
    __outb(PIC1_DATA, 0xff);
    __io_wait();
    __outb(PIC2_DATA, 0xff);
    __io_wait();
}
