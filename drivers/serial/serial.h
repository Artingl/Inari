#pragma once

#define SERIAL_PORT(port, reg) (port + reg)

#define SERIAL_COM0 0x3F8
#define SERIAL_COM1 0x2F8
#define SERIAL_COM2 0x3E8
#define SERIAL_COM3 0x2E8
#define SERIAL_COM4 0x5F8
#define SERIAL_COM5 0x4F8
#define SERIAL_COM6 0x3F8
#define SERIAL_COM7 0x5E8
#define SERIAL_COM8 0x4E8

#define SERIAL_DATA          0x00
#define SERIAL_INT_ENABLE    0x01
#define SERIAL_DIVl          0x00 // divider lest significant byte
#define SERIAL_DIVh          0x01 // divider most significant byte
#define SERIAL_FIFO          0x02
#define SERIAL_LINE_CONTROL  0x03
#define SERIAL_MODEM_CONTROL 0x04
#define SERIAL_LINE_STATUS   0x05
#define SERIAL_MODEM_STATUS  0x06
#define SERIAL_SCRATCH       0x07

#include <kernel/include/C/typedefs.h>

enum {
    SERIAL_SUCCESS = 0,
    SERIAL_PORT_FAULTY = 1,
};

int serial_init(uint16_t port, uint32_t baud);
uint8_t serial_read(uint16_t port);
void serial_write(uint16_t port, const char *ptr, size_t len);
void serial_putc(uint16_t port, uint8_t c);
void serial_printf(uint16_t port, const char *fmt, ...);
