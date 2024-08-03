#include <kernel/kernel.h>
#include <kernel/driver/serial/serial.h>

#include <kernel/arch/i686/impl.h>

#define SERIAL_PORT(port, reg) (port + reg)

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

#define serial_received(port) (__inb(SERIAL_PORT(port, SERIAL_LINE_STATUS)) & 1)
#define serial_is_transmit_empty(port) (__inb(SERIAL_PORT(port, SERIAL_LINE_STATUS)) & 0x20)

inline uint16_t serial_portnum_to_id(uint16_t num)
{
    switch (num)
    {
    case SERIAL_COM0: return 0x3F8;
    case SERIAL_COM1: return 0x2F8;
    case SERIAL_COM2: return 0x3E8;
    case SERIAL_COM3: return 0x2E8;
    case SERIAL_COM4: return 0x5F8;
    case SERIAL_COM5: return 0x4F8;
    case SERIAL_COM6: return 0x3F8;
    case SERIAL_COM7: return 0x5E8;
    case SERIAL_COM8: return 0x4E8;
    }

    return num;
}

uint8_t arch_serial_read(uint16_t port)
{
    port = serial_portnum_to_id(port);
    while(serial_received(port) == 0);
    uint8_t in = __inb(port);
    return in;
}

void arch_serial_putc(uint16_t port, uint8_t c)
{
    port = serial_portnum_to_id(port);
    while (serial_is_transmit_empty(port) == 0);
    __outb(port, c);
}

void arch_serial_write(uint16_t port, const char *ptr, size_t len)
{
    port = serial_portnum_to_id(port);
    for (size_t i = 0; i < len; i++)
        serial_putc(port, *(ptr++));
}

int arch_serial_init(uint16_t port, uint32_t baud)
{
    port = serial_portnum_to_id(port);
    
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
