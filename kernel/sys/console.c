#include <kernel/sys/console.h>
#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/interrupts/irq.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/proc/sched.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/timer.h>

#include <kernel/sync/spinlock.h>
#include <misc/list.h>
#include <misc/string.h>
#include <misc/types.h>

static uint16_t last_tty_id = 0;
static spinlock_t global_lock = {0}; /* TODO: global lock is awful */
static LIST_HEAD(ttys);

static tid_t console_task_id;
static uint16_t free_pool_index = 0;
static struct console_lazy_buffer console_pool[CONFIG_CONSOLE_POOL_SZ];
static uint16_t free_pool_entries[CONFIG_CONSOLE_POOL_SZ];
static uint8_t pool_ready = 0, is_early = 1;
static uint8_t does_tty0_exist = 0;
static struct console_tty tty0_block;

static void console_pool_init(void) {
    for (size_t i = 0; i < CONFIG_CONSOLE_POOL_SZ; i++) {
        free_pool_entries[i] = i;
    }
    free_pool_index = CONFIG_CONSOLE_POOL_SZ;
    pool_ready = 1;
}

static int console_buf_alloc(struct console_lazy_buffer **result) {
    if (!result)
        return -EINVAL;
    if (!pool_ready)
        console_pool_init();
    if (free_pool_index == 0)
        return -ENOMEM;

    free_pool_index--;
    uint16_t index = free_pool_entries[free_pool_index];
    *result = &console_pool[index];
    (*result)->timeout = uptime_us() + 1000000; // max 1 second
    return 0;
}

static int console_buf_free(struct console_lazy_buffer *buf) {
    if (!buf)
        return -EINVAL;

    if (free_pool_index >= CONFIG_CONSOLE_POOL_SZ) {
        kprintf("console: invalid console_lazy_buffer stack entry.");
        return -EINVAL;
    }
    /* Free it */
    uint16_t index = buf - console_pool;
    free_pool_entries[free_pool_index] = index;
    free_pool_index++;
    return 0;
}

static struct console_tty *get_tty(uint16_t tty_id) {
    if (!does_tty0_exist) {
        last_tty_id++;
        memset(&tty0_block, 0, sizeof(tty0_block));
        tty0_block.id = 0;
        tty0_block.owner = 0;
        INIT_LIST_HEAD(&tty0_block.lazy_list);
        list_add(&tty0_block.list, &ttys);
        does_tty0_exist = 1;
    }

    struct list_head *pos;
    struct console_tty *entry;
    list_for_each(pos, &ttys) {
        entry = list_entry(pos, struct console_tty, list);
        if (entry->id == tty_id) {
            return entry;
        }
    }

    return NULL;
}

/* Global lock must be locked by this point! */
static void flush_tty(struct console_tty *tty) {
    struct list_head *pos, *n;
    struct console_lazy_buffer *entry;
    struct device *chrdev;
    size_t i;

    /* Skip flushing if no devs to flush into */
    for (i = 0; i < CONSOLE_TTY_MAX_DEV; i++) {
        if (tty->devices[i].used)
            goto found_dev;
    }

    return;

found_dev:
    list_for_each_safe(pos, n, &tty->lazy_list) {
        entry = list_entry(pos, struct console_lazy_buffer, list);

        for (i = 0; i < CONSOLE_TTY_MAX_DEV; i++) {
            if (!tty->devices[i].used)
                continue;
            chrdev = char_get(tty->devices[i].dev);
            if (!chrdev) {
                /* Ooopsy */
                tty->devices[i].used = 0;
                continue;
            }

            ((struct char_ops *)chrdev->ops)->write(chrdev, (const uint8_t *)entry->buffer, entry->buffer_offset);
        }

        list_del(pos);
        console_buf_free(entry);
    }

    /* And dont forget latest buffer! */
    if (tty->latest_buffer) {
        for (i = 0; i < CONSOLE_TTY_MAX_DEV; i++) {
            if (!tty->devices[i].used)
                continue;
            chrdev = char_get(tty->devices[i].dev);
            if (!chrdev) {
                /* Ooopsy */
                tty->devices[i].used = 0;
                continue;
            }

            ((struct char_ops *)chrdev->ops)->write(
                chrdev, (const uint8_t *)tty->latest_buffer->buffer, tty->latest_buffer->buffer_offset);
        }

        console_buf_free(tty->latest_buffer);
        tty->latest_buffer = NULL;
    }
}

static void console_flush_buffer() {
    struct list_head *pos;
    struct console_tty *entry;
    unsigned long flags;
    spin_lock_irqsave(&global_lock, flags);

    /* TODO: cleanup timed out console lazy buffers */

    list_for_each(pos, &ttys) {
        entry = list_entry(pos, struct console_tty, list);
        flush_tty(entry);
    }

    spin_unlock_irqrestore(&global_lock, flags);
}

static void console_thread(void *arg) {
    /* Duty of this thread: sweep and dealloc all timed-out buffers in lazy_buffs of console ttys,
     * broadcast buffs to their devices, do console_buf_free */
    console_switch_normal();
    kprintf("console: thread is active.");

    while (1) {
        /* If there's nothing to do, wait a bit to avoid busy spin. */
        /* TODO: semaphore */
        console_flush_buffer();

        timer_usleep(1000);
    }
}

static int console_ioctl(struct device *chardev, unsigned long req, void *arg) {
    if (!chardev->driver_data)
        return -EINVAL;
    struct console_tty *tty = chardev->driver_data;

    switch (req) {
    case CONSOLE_IOCTL_TTY_ATTACH_DEV:
        return console_add_device(tty->id, (dev_t)arg);
    case CONSOLE_IOCTL_TTY_DETACH_DEV:
        return console_free_device(tty->id, (dev_t)arg);
    }

    return -ENOSYS;
}

static int console_write_char(struct device *chardev, const uint8_t *buf, size_t sz) {
    if (!chardev->driver_data)
        return -EINVAL;
    struct console_tty *tty = chardev->driver_data;
    console_write(tty->id, (const char*)buf, sz);
    return 0;
}

static struct char_ops console_ops = {
    .write = console_write_char,
    .ioctl = console_ioctl};

int console_init(void) {
    int ret;
    if ((ret = sched_create_thread("console", &console_task_id, &console_thread, NULL, NULL, NULL)) != 0)
        return ret;

    /* We now have all the required subsystems, finish tty0 initialization */
    return register_chardev(TTY_DRIVER, &console_ops, &tty0_block, &tty0_block.dev);
}

/* Printk/Panic, goes to tty0 */
int console_write_system(const char *s, size_t sz) { return console_write(0, s, sz); }

/* Tie device (vga/serial/vconsole) with tty */
int console_add_device(uint16_t tty_id, dev_t dev) {
    struct console_tty *tty;
    unsigned long flags;
    size_t i;

    /* Only char devices */
    struct device *chrdev = char_get(dev);
    if (!chrdev) {
        return -ENODEV;
    }

    spin_lock_irqsave(&global_lock, flags);
    if (!(tty = get_tty(tty_id))) {
        spin_unlock_irqrestore(&global_lock, flags);
        return -ENODEV;
    }
    for (i = 0; i < CONSOLE_TTY_MAX_DEV; i++) {
        if (!tty->devices[i].used)
            goto found;
    }

    spin_unlock_irqrestore(&global_lock, flags);
    return -EIO;

found:
    tty->devices[i].used = 1;
    tty->devices[i].dev = dev;

    spin_unlock_irqrestore(&global_lock, flags);
    kprintf("console: dev:%s%d attached to tty %u", chrdev->group->name, DEVID(chrdev->dev), tty_id);
    return 0;
}

int console_free_device(uint16_t tty_id, dev_t dev) {
    struct console_tty *tty;
    unsigned long flags;
    size_t i;

    spin_lock_irqsave(&global_lock, flags);
    if (!(tty = get_tty(tty_id))) {
        spin_unlock_irqrestore(&global_lock, flags);
        return -ENODEV;
    }
    for (i = 0; i < CONSOLE_TTY_MAX_DEV; i++) {
        if (tty->devices[i].used && tty->devices[i].dev == dev) {
            tty->devices[i].used = 0;
            goto found;
        }
    }

    spin_unlock_irqrestore(&global_lock, flags);
    return -EIO;

found:
    spin_unlock_irqrestore(&global_lock, flags);
    kprintf("console: dev:%d detached from tty %u", dev, tty_id);
    return 0;
}

/* Allocates new tty, returns the id, registers chardev */
int console_alloc_tty(pid_t owner, uint16_t *tty_id) {
    struct console_tty *tty = kmalloc(sizeof(struct console_tty));
    if (!tty)
        return -ENOMEM;
    unsigned long flags;
    spin_lock_irqsave(&global_lock, flags);
    tty->id = last_tty_id++;
    tty->owner = owner;
    INIT_LIST_HEAD(&tty->lazy_list);
    register_chardev(TTY_DRIVER, &console_ops, tty, &tty->dev);
    list_add(&tty->list, &ttys);
    if (tty_id)
        *tty_id = tty->id;
    spin_unlock_irqrestore(&global_lock, flags);
    return 0;
}

/* Frees tty N. Check that the caller is the owner of this tty AND it is not tty0 */
int console_free_tty(pid_t caller, uint16_t tty_id) {
    if (tty_id == 0)
        return -EINVAL;
    unsigned long flags;
    struct console_tty *tty;
    spin_lock_irqsave(&global_lock, flags);
    if (!(tty = get_tty(tty_id))) {
        spin_unlock_irqrestore(&global_lock, flags);
        return -ENODEV;
    }
    if (tty->owner != caller) {
        spin_unlock_irqrestore(&global_lock, flags);
        return -EACCES;
    }
    unregister_chardev(tty->dev);
    list_del(&tty->list);
    kfree(tty);
    spin_unlock_irqrestore(&global_lock, flags);
    return 0;
}

/* Cleans up ttys for a given proc PID */
int console_cleanup_proc(struct process *target) {
    struct list_head *pos, *n;
    struct console_tty *entry;
    unsigned long flags;
    spin_lock_irqsave(&global_lock, flags);
    list_for_each_safe(pos, n, &ttys) {
        entry = list_entry(pos, struct console_tty, list);
        if (entry->owner == target->pid) {
            unregister_chardev(entry->dev);
            list_del(pos);
            kfree(entry);
        }
    }

    spin_unlock_irqrestore(&global_lock, flags);
    return 0;
}

/* Write to tty N. Can later be read via the function below */
int console_write(uint16_t tty_id, const char *s, size_t sz) {
    unsigned long flags;
    struct console_tty *tty;

    /* Enqueue characters into buffers */
    if (!is_early) spin_lock_irqsave(&global_lock, flags);
    if (!(tty = get_tty(tty_id))) {
        if (!is_early) spin_unlock_irqrestore(&global_lock, flags);
        return -ENODEV;
    }

    while (sz > 0) {
        /* ensure we have a current buffer */
        if (!tty->latest_buffer) {
            if (console_buf_alloc(&tty->latest_buffer) != 0) {
                /* Drop the data, we don't have space */
                break;
            }
            tty->latest_buffer->buffer_offset = 0;
        }

        uint32_t space = CONFIG_CONSOLE_BUFF_SZ - tty->latest_buffer->buffer_offset;
        uint32_t to_copy = (sz < space) ? sz : space;

        memcpy(&tty->latest_buffer->buffer[tty->latest_buffer->buffer_offset], s, to_copy);

        tty->latest_buffer->buffer_offset += to_copy;
        s += to_copy;
        sz -= to_copy;

        /* if current buffer full, move it to lazy list */
        if (tty->latest_buffer->buffer_offset >= CONFIG_CONSOLE_BUFF_SZ) {
            list_add_tail(&tty->latest_buffer->list, &tty->lazy_list);
            tty->latest_buffer = NULL;
        }
    }

    /* Flush right here if early */
    if (is_early) {
        flush_tty(tty);
    }

    if (!is_early) spin_unlock_irqrestore(&global_lock, flags);
    return 0;
}

/* Read from tty N (its contents) */
int console_read(uint16_t tty_id, const char *s, size_t sz) { return -1; }

/* Writes input into tty ring, which can later be read from getc ioctl */
int console_write_in_queue(uint16_t tty_id, struct console_in in) {
    unsigned long flags;
    struct console_tty *tty;
    spin_lock_irqsave(&global_lock, flags);
    if (!(tty = get_tty(tty_id))) {
        spin_unlock_irqrestore(&global_lock, flags);
        return -ENODEV;
    }

    tty->in_ring.in[tty->in_ring.head] = in;
    if (tty->in_ring.tail > tty->in_ring.head) {
        tty->in_ring.head++;
        tty->in_ring.tail++;
        if (tty->in_ring.tail >= sizeof(tty->in_ring.in) / sizeof(struct console_in)) {
            tty->in_ring.tail = 0;
        }
    } else {
        tty->in_ring.head++;
        if (tty->in_ring.head >= sizeof(tty->in_ring.in) / sizeof(struct console_in)) {
            tty->in_ring.tail++;
            tty->in_ring.head = 0;
        }
    }
    spin_unlock_irqrestore(&global_lock, flags);
    return 0;
}

void console_switch_normal(void) { is_early = 0; }

void console_switch_early(void) { is_early = 1; }

int console_is_early(void) { return is_early; }
