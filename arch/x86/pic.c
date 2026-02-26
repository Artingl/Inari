#include <kernel/inari.h>

#include <misc/string.h>

#include <arch/x86/acpi.h>
#include <arch/x86/cpu.h>
#include <arch/x86/pic.h>
#include <arch/x86/arch.h>

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x28

#define PIC1	            0x20		/* IO base address for master PIC */
#define PIC2	            0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND        PIC1
#define PIC1_DATA           (PIC1+1)
#define PIC2_COMMAND        PIC2
#define PIC2_DATA           (PIC2+1)

#define ICW1_ICW4           0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE         0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4      0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL          0x08		/* Level triggered (edge) mode */
#define ICW1_INIT           0x10		/* Initialization - required! */
 
#define ICW4_8086           0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO           0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE      0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER     0x0C		/* Buffered mode/master */
#define ICW4_SFNM           0x10		/* Special fully nested (not) */


static uint8_t pic_initialized;

int x86_pic_init(void)
{
    /* Reset PICs */
    x86_outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    x86_io_wait();
    x86_outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    x86_io_wait();

    /* Set PIC offsets */
    x86_outb(PIC1_DATA, PIC1_OFFSET);
    x86_io_wait();
    x86_outb(PIC2_DATA, PIC2_OFFSET);
    x86_io_wait();

    /* Tell PICs whose of them is slave and master */
    x86_outb(PIC1_DATA, 4);
    x86_io_wait();
    x86_outb(PIC2_DATA, 2);
    x86_io_wait();

    /* Tell them to be in 8086 mode */
    x86_outb(PIC1_DATA, ICW4_8086);
    x86_io_wait();
    x86_outb(PIC2_DATA, ICW4_8086);
    x86_io_wait();

    /* Set masks */
    x86_outb(PIC1_DATA, 0x00);
    x86_io_wait();
    x86_outb(PIC2_DATA, 0x00);
    x86_io_wait();

    x86_pic_irq_unmask(4);
    x86_pic_irq_unmask(2);
    x86_pic_irq_unmask(12);
    x86_pic_irq_mask(14);
    x86_pic_irq_mask(15);

    pic_initialized = 1;

    kprintf("pic: initialized");
    return 0;
}

void x86_pic_acknowledge(uint8_t irq_no)
{
    /* If the interrupt vector came from the Slave PIC's range */
    if (irq_no >= PIC2_OFFSET && irq_no <= (PIC2_OFFSET + 7)) {
        x86_outb(PIC2_COMMAND, 0x20);
    }
    
    x86_outb(PIC1_COMMAND, 0x20);
}

void x86_pic_irq_unmask(uint8_t irq_line)
{
    uint16_t port;
    uint8_t value;
    kprintf("pic: irq unmask %d", irq_line);

    if (irq_line < 8) port = PIC1_DATA;
    else
    {
        port = PIC2_DATA;
        irq_line -= 8;
    }

    value = x86_inb(port) & ~(1 << irq_line);
    x86_outb(port, value);
}


void x86_pic_irq_mask(uint8_t irq_line)
{
    uint16_t port;
    uint8_t value;
    kprintf("pic: irq mask %d", irq_line);

    if (irq_line < 8) port = PIC1_DATA;
    else
    {
        port = PIC2_DATA;
        irq_line -= 8;
    }

    value = x86_inb(port) | (1 << irq_line);
    x86_outb(port, value);
}