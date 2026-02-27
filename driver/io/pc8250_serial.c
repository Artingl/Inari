#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_PC8250_SERIAL

#include <kernel/console/console.h>
#include <kernel/console/earlycon.h>
#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/interrupts/irq.h>
#include <kernel/module.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>

#include <misc/ring.h>
#include <misc/string.h>

#include <arch/sys.h>
#include <arch/x86/arch.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8
#define COM5 0x5F8
#define COM6 0x4F8
#define COM7 0x5E8
#define COM8 0x4E8

#define SERIAL_IRQ 0x4
#define is_transmit_empty(port) (x86_inb(port + 5) & 0x20)

extern int console_is_early;
static int serial_is_initialized = 0;
static int console_port = COM1;

static uint8_t buffer[256];
static RING_HEAD(tx_buff, &buffer, 256);

static int serial_irq_handler(uint32_t irq, void *driver_data) {
    if (console_is_early || !serial_is_initialized)
        return IRQ_HANDLED;

    int port = console_port;
    uint8_t in = x86_inb(console_port + 2), data;

    if (in & 0x01)
        return IRQ_HANDLED;
    if ((in & 0x0F) == 0x02) {
        if (ring_bbuf_is_empty(&tx_buff))
            x86_outb(console_port + 1, 0x01);
        else {
            if (ring_bbuf_pop(&tx_buff, &data) == 0)
                x86_outb(port, data);
        }
    } else if ((in & 0x0F) == 0x04 || (in & 0x0F) == 0x0C)
        x86_inb(port);

    return IRQ_HANDLED;
}

static void serial_puts(int port, const char *s, uint32_t count) {
    if (!serial_is_initialized || port != console_port || count <= 0)
        return;
    // if (!console_is_early)
    // {
    //     int was_empty = ring_bbuf_is_empty(&tx_buff);
    //     uint8_t data;
    //     while (count--)
    //         ring_bbuf_push(&tx_buff, *s++);
    //     if (was_empty && ring_bbuf_pop(&tx_buff, &data) == 0) {
    //         x86_outb(port, data);
    //     }
    //     x86_outb(port + 1, 0x03);
    // }
    // else {
    /* TODO: for some reason buffered serial breaks; just doesn't fire interrupts */
    while (is_transmit_empty(port) == 0)
        ;
    while (count--)
        x86_outb(port, *s++);
    // }
}

static void serial_puts_console(const char *s, uint32_t count) {
    if (!serial_is_initialized)
        return;
    serial_puts(console_port, s, count);
}

static void serial_flush_console() {
    if (!serial_is_initialized)
        return;
    x86_outb(console_port, 0);
    x86_outb(console_port + 1, 0x03);
}

static struct console_dev console_dev = {.name = "pc8250_serial",
                                         .write = serial_puts_console,
                                         .flush = serial_flush_console,
                                         .rewind = NULL,
                                         .read = NULL,
                                         .flags = CONSOLE_EARLY | CONSOLE_PRINT};

static int pc8250_serial_init() {
    if (serial_is_initialized)
        return 0;

    char device[ARG_MAX_LEN];
    parse_cmdline_argument("console_port", &device[0]);

    /* Set the port based on provided arguments */
    console_port = COM1;
    if (strcmp(device, "COM2") == 0)
        console_port = COM2;
    if (strcmp(device, "COM3") == 0)
        console_port = COM3;
    if (strcmp(device, "COM4") == 0)
        console_port = COM4;
    if (strcmp(device, "COM5") == 0)
        console_port = COM5;
    if (strcmp(device, "COM6") == 0)
        console_port = COM6;
    if (strcmp(device, "COM7") == 0)
        console_port = COM7;
    if (strcmp(device, "COM8") == 0)
        console_port = COM8;

    x86_outb(console_port + 1, 0x00); // Disable all interrupts
    x86_outb(console_port + 3, 0x80); // Enable DLAB (set baud rate divisor)
    x86_outb(console_port + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    x86_outb(console_port + 1, 0x00); //                  (hi byte)
    x86_outb(console_port + 3, 0x03); // 8 bits, no parity, one stop bit
    x86_outb(console_port + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    x86_outb(console_port + 4, 0x0B); // IRQs enabled, RTS/DSR set
    x86_outb(console_port + 1, 0x01);
    x86_outb(console_port + 4, 0x1E); // Set in loopback mode, test the serial chip
    x86_outb(console_port + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial returns same byte)

    if (x86_inb(console_port + 0) != 0xAE) {
        return -ENODEV;
    }
    x86_outb(console_port + 4, 0x0F);
    serial_is_initialized = 1;
    return 0;
}

static int serial_write_chardev(struct device *chardev, const uint8_t *buf, size_t sz) {
    if (!buf)
        return -EINVAL;
    serial_puts((int)chardev->driver_data, (const char *)buf, sz);
    return sz;
}

static struct char_ops serial_block_ops = {.write = &serial_write_chardev};

static dev_t char_devices[8] = {0};

static int pc8250_serial_probe() {
    if (pc8250_serial_init() != 0)
        return -ENODEV;
    console_register(&console_dev);
    irq_request(SERIAL_IRQ, &serial_irq_handler, NULL);

    if (serial_is_initialized) {
        register_chardev(SERIAL_DRIVER, &serial_block_ops, (void *)console_port, &char_devices[0]);
        // register_chardev(SERIAL_DRIVER, &serial_block_ops, (void*)COM1, &char_devices[0]);
        // register_chardev(SERIAL_DRIVER, &serial_block_ops, (void*)COM2, &char_devices[1]);
        // register_chardev(SERIAL_DRIVER, &serial_block_ops, (void*)COM3, &char_devices[2]);
        // register_chardev(SERIAL_DRIVER, &serial_block_ops, (void*)COM4, &char_devices[3]);
        // register_chardev(SERIAL_DRIVER, &serial_block_ops, (void*)COM5, &char_devices[4]);
        // register_chardev(SERIAL_DRIVER, &serial_block_ops, (void*)COM6, &char_devices[5]);
        // register_chardev(SERIAL_DRIVER, &serial_block_ops, (void*)COM7, &char_devices[6]);
        // register_chardev(SERIAL_DRIVER, &serial_block_ops, (void*)COM8, &char_devices[7]);
    }

    return 0;
}

static int pc8250_serial_early_probe() {
    if (pc8250_serial_init() != 0)
        return -ENODEV;
    console_register(&console_dev);
    return 0;
}

static void pc8250_serial_cleanup() {
    size_t i;
    if (serial_is_initialized) {
        unregister_chardev(char_devices[0]);
        // unregister_chardev(char_devices[1]);
        // unregister_chardev(char_devices[2]);
        // unregister_chardev(char_devices[3]);
        // unregister_chardev(char_devices[4]);
        // unregister_chardev(char_devices[5]);
        // unregister_chardev(char_devices[6]);
        // unregister_chardev(char_devices[7]);
        irq_free(SERIAL_IRQ, &serial_irq_handler);
        for (i = 0; i < 8; i++)
            char_devices[i] = i;
        tx_buff = RING_HEAD_INIT(&buffer, 256);
        serial_is_initialized = 0;
    }
}

static void pc8250_serial_early_cleanup() {}

earlycon_device("pc8250", pc8250_serial_early_probe, pc8250_serial_early_cleanup);

module_t pc8250_module = {
    .probe = pc8250_serial_probe,
    .cleanup = pc8250_serial_cleanup,
};

/* This would allow to register this device as normal console after early stage.
 * If it was already initialized, nothing should happen. */
module_register("pc8250", pc8250_module);

#endif
#endif
