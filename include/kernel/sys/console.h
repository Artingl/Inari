#ifndef _INARI_CONSOLE_H
#define _INARI_CONSOLE_H

#include <kernel/sync/spinlock.h>
#include <kernel/sys/device.h>
#include <kernel/proc/proc.h>

#include <misc/list.h>
#include <misc/types.h>

#include <kernel/inari.h>
#include <misc/types.h>

#define CONSOLE_EARLY 1 << 0
#define CONSOLE_PRINT 1 << 1
#define CONSOLE_PANIC 1 << 2
#define CONSOLE_DEBUG 1 << 3

#define CONSOLE_IOCTL_FLUSH          0
#define CONSOLE_IOCTL_TTY_ATTACH_DEV 1 // ioctl arg here is the vfs handle to a device we need to use
#define CONSOLE_IOCTL_TTY_DETACH_DEV 2 // ioctl arg here is the vfs handle to a device we need to use

#define CONSOLE_TTY_MAX_DEV 6

struct console_in {
    uint8_t pressed;
    uint8_t modifier;
    uint16_t key;
    uint16_t chr;
} __attribute__((packed));

struct console_lazy_buffer {
    char buffer[CONFIG_CONSOLE_BUFF_SZ];
    uint32_t buffer_offset;
    time_t timeout;
    struct list_head list;
};

struct console_tty {
    uint16_t id;
    spinlock_t lock;
    struct {
        uint8_t head;
        uint8_t tail;
        struct console_in in[8];
    } in_ring;

    struct console_lazy_buffer *latest_buffer;
    struct list_head lazy_list;

    struct {
        uint8_t used;
        dev_t dev;
    } devices[CONSOLE_TTY_MAX_DEV];

    dev_t dev;
    pid_t owner;

    struct list_head list;
};

int console_init(void);

/* Printk/Panic, goes to tty0 */
int console_write_system(const char *s, size_t sz);

/* Tie device (vga/serial/vconsole) with tty */
int console_add_device(uint16_t tty_id, dev_t dev);

int console_free_device(uint16_t tty_id, dev_t dev);

/* Allocates new tty, returns the id, registers chardev */
int console_alloc_tty(pid_t owner, uint16_t *tty_id);

/* Frees tty N. Checks that the caller is the owner of this tty AND it is not tty0 */
int console_free_tty(pid_t caller, uint16_t tty_id);

/* Cleans up ttys for a given proc PID */
int console_cleanup_proc(struct process *target);

/* Write to tty N. Can later be read via the function below */
int console_write(uint16_t tty_id, const char *s, size_t sz);

/* Read from tty N (its contents) */
int console_read(uint16_t tty_id, const char *s, size_t sz);

/* Writes input into tty ring, which can later be read from getc ioctl */
int console_write_in_queue(uint16_t tty_id, struct console_in in);

int console_is_early(void);
void console_switch_normal(void);
void console_switch_early(void);

#endif
