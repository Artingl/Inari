#ifndef _INARI_CONSOLE_H
#define _INARI_CONSOLE_H

#include <stddef.h>
#include <stdint.h>

#define CONSOLE_EARLY  1 << 0
#define CONSOLE_PRINTK 1 << 1

typedef void (*console_putc)(char c);

typedef struct {
    const char *name;
    console_putc putc;

    uint32_t flags;

} console_dev_t;

extern int console_register(console_dev_t *dev);
extern int console_unregister(console_dev_t *dev);

#endif