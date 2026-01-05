#ifndef _INARI_CONSOLE_H
#define _INARI_CONSOLE_H

#include <misc/types.h>
#include <misc/list.h>

#define CONSOLE_EARLY  1 << 0
#define CONSOLE_PRINTK 1 << 1
#define CONSOLE_PANIC  1 << 2
#define CONSOLE_DEBUG  1 << 3

typedef void (*console_io)(const char *s, uint32_t count);

typedef struct console_dev {
    const char *name;
    console_io write;
    console_io read;
    uint32_t flags;

    struct list_head list;
} console_dev_t;

int console_init(void);
int console_register(struct console_dev *dev);
int console_unregister(struct console_dev *dev);

/* Switch back to early console */
void console_switch_early(void);

#define CONSOLE_MESSAGE_PRINTK 0x00
#define CONSOLE_MESSAGE_DEBUG  0x01

int console_printc(int type, const char *s, uint32_t count);

#endif