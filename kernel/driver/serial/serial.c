#include <kernel/kernel.h>
#include <kernel/driver/serial/serial.h>
#include <kernel/include/io.h>

#include <stdarg.h>

// Every architecture must have an implemented serial driver with these following functions
extern int arch_serial_init(uint16_t port, uint32_t baud);
extern void arch_serial_putc(uint16_t port, uint8_t c);
extern void arch_serial_write(uint16_t port, const char *ptr, size_t len);
extern uint8_t arch_serial_read(uint16_t port);

int serial_init(uint16_t port, uint32_t baud)
{
    return arch_serial_init(port, baud);
}

static int __serial_printf_handler(char c, void **p)
{
    arch_serial_putc(**((uint16_t**)p), c);
    return 0;
}

void serial_write(uint16_t port, const char *ptr, size_t len)
{
    arch_serial_write(port, ptr, len);
}

void serial_putc(uint16_t port, uint8_t c)
{
    arch_serial_putc(port, c);
}

uint8_t serial_read(uint16_t port)
{
    return arch_serial_read(port);
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