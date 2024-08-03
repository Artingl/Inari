#pragma once

#include <kernel/include/typedefs.h>
#include <kernel/core/lock/spinlock.h>

#include <kernel/driver/serial/serial.h>

#define IS_UNICODE(c) (((c) & 0xc0) == 0xc0)

#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 30

#define CONSOLE_TAB_SIZE 4

#define CONSOLE_SERIAL_BAUD 9600

typedef struct console {
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t buffer_width;
    uint32_t buffer_height;
    uint16_t serial_port;

    uint8_t *buffer;
    uint8_t unicode_bytes;
    uint8_t unicode_bytes_start;

    int is_unicode;
    int heap_allocated;
    int must_flush;

    spinlock_t spinlock;

#define CONSOLE_LINE_UPDATED (1 << 0)
    uint32_t *lines_state;

} console_t;

void console_late_init();
void console_init(uint16_t serial_port);

void console_clear();
void console_flush();

int console_print(const char *msg);
int console_printc(char c);
