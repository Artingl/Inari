#pragma once

#include <kernel/include/C/typedefs.h>

#include <drivers/serial/serial.h>

#define CONSOLE_TAB_SIZE 4

#define CONSOLE_SERIAL_BAUD 9600
#define CONSOLE_SERIAL_PORT SERIAL_COM0

struct console {
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t buffer_width;
    uint32_t buffer_height;

    uint32_t *buffer;
    
    uint8_t unicode_bytes;
    uint8_t unicode_bytes_start;
    bool is_unicode;

#define CONSOLE_LINE_UPDATED (1 << 0)

    uint32_t *lines_state;

};

void sys_console_enable_heap();
void sys_console_update_video();
void sys_console_init();

void sys_console_use(struct console *console);
void sys_console_render();
void sys_console_flush();

int sys_console_print(const char *msg);
int sys_console_printc(char c);
