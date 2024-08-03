#pragma once

#define SERIAL_COM0 0x01
#define SERIAL_COM1 0x02
#define SERIAL_COM2 0x03
#define SERIAL_COM3 0x04
#define SERIAL_COM4 0x05
#define SERIAL_COM5 0x06
#define SERIAL_COM6 0x07
#define SERIAL_COM7 0x08
#define SERIAL_COM8 0x09

#include <kernel/include/C/typedefs.h>

enum {
    SERIAL_SUCCESS = 0,
    SERIAL_PORT_FAULTY = 1,
};

int serial_init(uint16_t port, uint32_t baud);
void serial_write(uint16_t port, const char *ptr, size_t len);
void serial_putc(uint16_t port, uint8_t c);
uint8_t serial_read(uint16_t port);

void serial_printf(uint16_t port, const char *fmt, ...);
