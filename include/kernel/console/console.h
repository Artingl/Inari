#ifndef _INARI_CONSOLE_H
#define _INARI_CONSOLE_H

#include <misc/list.h>
#include <misc/types.h>

#define CONSOLE_EARLY 1 << 0
#define CONSOLE_PRINT 1 << 1
#define CONSOLE_PANIC 1 << 2
#define CONSOLE_DEBUG 1 << 3

#define CONSOLE_IOCTL_REWIND 0     // Rewind N chars in the console buffer
#define CONSOLE_IOCTL_REWIND_CLR 1 // Rewind N chars in the console buffer AND clear them
#define CONSOLE_IOCTL_CLR 2        // Clear
#define CONSOLE_IOCTL_FLUSH 3

typedef void (*console_io)(const char *s, uint32_t count);

typedef struct console_dev {
    const char *name;
    void (*write)(const char *s, uint32_t count);
    void (*read)(const char *s, uint32_t count);
    void (*rewind)(uint32_t count, int clear);
    void (*clear)();
    void (*flush)();
    uint32_t flags;

    struct list_head list;
} console_dev_t;

int console_init(void);
int console_register(struct console_dev *dev);
int console_unregister(struct console_dev *dev);

/* Switch back to early console */
void console_switch_early(void);
void console_switch_normal(void);

#define CONSOLE_MESSAGE_KPRINTF 0x00
#define CONSOLE_MESSAGE_DEBUG 0x01

int console_puts(int type, const char *s, uint32_t count);
void console_flush(void);

#endif