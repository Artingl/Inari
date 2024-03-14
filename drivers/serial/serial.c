#include <kernel/kernel.h>

#include <drivers/serial/serial.h>
#include <drivers/impl.h>

#include <kernel/include/C/io.h>

#include <stdarg.h>

#define serial_received(port) (__inb(SERIAL_PORT(port, SERIAL_LINE_STATUS)) & 1)
#define serial_is_transmit_empty(port) (__inb(SERIAL_PORT(port, SERIAL_LINE_STATUS)) & 0x20)

int serial_init(uint16_t port, uint32_t baud)
{
    // disable interrupts
    __outb(SERIAL_PORT(port, SERIAL_INT_ENABLE), 0x00);

    // set the divider value (with DLAB set first)
    uint16_t div = (uint16_t)(baud / 115200);
    __outb(SERIAL_PORT(port, SERIAL_LINE_CONTROL), 0x80);
    __outb(SERIAL_PORT(port, SERIAL_DIVl), div & 0xff);
    __outb(SERIAL_PORT(port, SERIAL_DIVh), div >> 8);
    __outb(SERIAL_PORT(port, SERIAL_LINE_CONTROL), 0x03);

    __outb(SERIAL_PORT(port, SERIAL_FIFO), 0xC7);          // Enable FIFO, clear them, with 14-byte threshold
    __outb(SERIAL_PORT(port, SERIAL_MODEM_CONTROL), 0x0B); // IRQs enabled, RTS/DSR set
    __outb(SERIAL_PORT(port, SERIAL_MODEM_CONTROL), 0x1E); // Set in loopback mode, test the serial chip
    __outb(SERIAL_PORT(port, SERIAL_DATA), 0xAE);          // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check the port
    if (__inb(SERIAL_PORT(port, SERIAL_DATA)) != 0xae)
    {
        return SERIAL_PORT_FAULTY;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    __outb(SERIAL_PORT(port, SERIAL_MODEM_CONTROL), 0x0F);
    return SERIAL_SUCCESS;
}

uint8_t serial_read(uint16_t port)
{
    while(serial_received(port) == 0);
    uint8_t in = __inb(port);
    return in;
}

void serial_putc(uint16_t port, uint8_t c)
{
    while (serial_is_transmit_empty(port) == 0);
    __outb(port, c);
}

void __serial_printf_handler(char c, void **p)
{
    serial_putc(**((uint16_t**)p), c);
}

void serial_write(uint16_t port, const char *ptr, size_t len)
{
    for (size_t i = 0; i < len; i++)
        serial_putc(port, *(ptr++));
}

void serial_printf(uint16_t port, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    serial_putc(port, '\r');
    serial_putc(port, '\n');
    do_printkn(fmt, args, &__serial_printf_handler, &port);
    serial_putc(port, '\r');
    serial_putc(port, '\n');
    va_end(args);
}