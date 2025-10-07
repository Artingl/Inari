#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_PC8250_SERIAL

#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/module.h>
#include <kernel/errno.h>

#include <misc/string.h>
#include <arch/x86/arch.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8
#define COM5 0x5F8
#define COM6 0x4F8
#define COM7 0x5E8
#define COM8 0x4E8

#define is_transmit_empty(port) (x86_inb(port + 5) & 0x20)

static int pc8250_is_initialized = 0;
static int pc8250_port = COM1;

static void serial_putc(const char *s, uint32_t count)
{
    if (!pc8250_is_initialized)
        return;
    while (is_transmit_empty(pc8250_port) == 0);
    while (count--)
        x86_outb(pc8250_port, *s++);
}

static struct console_dev console_dev = {
    .name = "pc8250_serial",
    .write = serial_putc,
    .read = NULL,
    .flags = CONSOLE_EARLY | CONSOLE_PRINTK
};

int pc8250_serial_init()
{
    if (pc8250_is_initialized)
        return 0;
    
    char device[ARG_MAX_LEN];
    parse_cmdline_argument("pc8250_port", &device[0]);

    /* Set the port based on provided arguments */
    pc8250_port = COM1;
    if (strcmp(device, "COM2") == 0) pc8250_port = COM2;
    if (strcmp(device, "COM3") == 0) pc8250_port = COM3;
    if (strcmp(device, "COM4") == 0) pc8250_port = COM4;
    if (strcmp(device, "COM5") == 0) pc8250_port = COM5;
    if (strcmp(device, "COM6") == 0) pc8250_port = COM6;
    if (strcmp(device, "COM7") == 0) pc8250_port = COM7;
    if (strcmp(device, "COM8") == 0) pc8250_port = COM8;

    x86_outb(pc8250_port + 1, 0x00);    // Disable all interrupts
    x86_outb(pc8250_port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    x86_outb(pc8250_port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    x86_outb(pc8250_port + 1, 0x00);    //                  (hi byte)
    x86_outb(pc8250_port + 3, 0x03);    // 8 bits, no parity, one stop bit
    x86_outb(pc8250_port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    x86_outb(pc8250_port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    x86_outb(pc8250_port + 4, 0x1E);    // Set in loopback mode, test the serial chip
    x86_outb(pc8250_port + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    if(x86_inb(pc8250_port + 0) != 0xAE) {
        return -ENODEV;
    }

    x86_outb(pc8250_port + 4, 0x0F);
    pc8250_is_initialized = 1;

    return 0;
}

int pc8250_serial_probe()
{
    if (pc8250_serial_init() != 0)
        return -ENODEV;
    
    return console_register(&console_dev);
}

void pc8250_serial_cleanup()
{
}

earlycon_device(
    "pc8250",
    pc8250_serial_probe,
    pc8250_serial_cleanup
);

/* This would allow to register this device as normal console after early stage.
 * If it was already initialized, nothing should happen. */
module_register(
    "pc8250",
    pc8250_serial_probe,
    pc8250_serial_cleanup
);

#endif
#endif
