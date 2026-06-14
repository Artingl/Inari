#include <kernel/console/console.h>
#include <kernel/console/earlycon.h>
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
static LIST_HEAD(devices);

static tid_t console_task_id;
static uint16_t free_pool_index = 0;
static struct console_lazy_buffer console_pool[CONFIG_CONSOLE_POOL_SZ];
static uint16_t free_pool_entries[CONFIG_CONSOLE_POOL_SZ];
static uint8_t pool_ready = 0, is_early = 1;

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
    if (free_pool_index == 0)
        return -ENOMEM;
    if (!pool_ready)
        console_pool_init();

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

    // if (!(tty = get_tty(tty_id))) {
    //     spin_unlock_irqrestore(&global_lock, flags);
    //     return -ENODEV;
    // }

    // while (sz > 0) {
    //     /* ensure we have a current buffer */
    //     if (!tty->latest_buffer) {
    //         if (console_buf_alloc(&tty->latest_buffer) != 0) {
    //             /* Drop the data, we don't have space */
    //             break;
    //         }
    //         tty->latest_buffer->buffer_offset = 0;
    //     }

    //     uint32_t space = CONFIG_CONSOLE_BUFF_SZ - tty->latest_buffer->buffer_offset;
    //     uint32_t to_copy = (sz < space) ? sz : space;

    //     memcpy(&tty->latest_buffer->buffer[tty->latest_buffer->buffer_offset], s, to_copy);

    //     tty->latest_buffer->buffer_offset += to_copy;
    //     s += to_copy;
    //     sz -= to_copy;

    //     /* if current buffer full, move it to lazy list */
    //     if (tty->latest_buffer->buffer_offset >= CONFIG_CONSOLE_BUFF_SZ) {
    //         list_add_tail(&tty->latest_buffer->list, &tty->lazy_list);
    //         tty->latest_buffer = NULL;
    //     }
    // }
}

static void console_flush_buffer() {
    struct list_head *pos;
    struct console_tty *entry;
    struct console_dev *entry_dev;
    unsigned long flags;
    spin_lock_irqsave(&global_lock, flags);

    /* TODO: cleanup timed out console lazy buffers */

    list_for_each(pos, &ttys) {
        entry = list_entry(pos, struct console_tty, list);
        flush_tty(entry);
    }

    list_for_each(pos, &devices) {
        entry_dev = list_entry(pos, struct console_dev, list);
        if (entry_dev->flush)
            entry_dev->flush();
    }

    spin_unlock_irqrestore(&global_lock, flags);
}

static void console_thread(void *arg) {
    /* Duty of this thread: sweep and dealloc all timed-out buffers in lazy_buffs of console ttys,
     * broadcast buffs to their devices, do console_buf_free */
    // console_switch_normal();
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
    uint16_t tty_id;
    if ((ret = sched_create_thread("console", &console_task_id, &console_thread, NULL, NULL, NULL)) != 0)
        return ret;

    /* Init tty0 */
    return console_alloc_tty(0, &tty_id);
}

/* Printk/Panic, goes to earlycon/tty0 */
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

int console_register(struct console_dev *dev) {
    struct console_dev *entry;
    struct list_head *pos;
    unsigned long flags;

    if (!dev)
        return -EINVAL;
    spin_lock_irqsave(&global_lock, flags);
    /* Check that we don't already have such device */
    if (dev->name)
        list_for_each(pos, &devices) {
            entry = list_entry(pos, struct console_dev, list);
            if (strcmp(dev->name, entry->name) == 0) {
                spin_unlock_irqrestore(&global_lock, flags);
                return -EBUSY;
            }
        }

    list_add(&dev->list, &devices);
    spin_unlock_irqrestore(&global_lock, flags);
    kprintf("console: new device %s registered", dev->name);
    return 0;
}

int console_unregister(struct console_dev *dev) {
    if (!dev)
        return -EINVAL;
    unsigned long flags;
    spin_lock_irqsave(&global_lock, flags);
    list_del(&dev->list);
    spin_unlock_irqrestore(&global_lock, flags);
    kprintf("console: device %s unregistered", dev->name);
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
    struct console_dev *entry;
    struct list_head *pos;

    if (is_early) {
        /* We ignore tty when early, just write into any available earlycon */
        list_for_each(pos, &devices) {
            entry = list_entry(pos, struct console_dev, list);

            if (!entry->write || !(entry->flags & CONSOLE_EARLY))
                continue;

            entry->write(s, sz);
            if (entry->flush)
                entry->flush();
        }
        return 0;
    }

    /* Enqueue characters into buffers */
    spin_lock_irqsave(&global_lock, flags);
    if (!(tty = get_tty(tty_id))) {
        spin_unlock_irqrestore(&global_lock, flags);
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

    spin_unlock_irqrestore(&global_lock, flags);
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

int console_flush(uint16_t tty_id) { return -1; }

void console_switch_normal(void) { is_early = 0; }

void console_switch_early(void) { is_early = 1; }

int console_is_early(void) { return is_early; }

// -----------

// struct console_lazy_buffer {
//     char buffer[CONFIG_CONSOLE_BUFF_SZ];
//     uint32_t buffer_offset;
//     struct list_head list;
//     uint8_t from_pool; /* 1 if from static pool, 0 if from kmalloc */
// };

// LIST_HEAD(consoles_list);
// LIST_HEAD(console_lazy_list); /* buffers awaiting printing */

// static struct console_lazy_buffer *console_latest_buffer = (struct console_lazy_buffer *)NULL;

// static spinlock_t console_lock = {0};

// static struct console_lazy_buffer console_pool[CONFIG_CONSOLE_POOL_SZ];
// static LIST_HEAD(console_pool_free_list);

// static dev_t console_dev = 0;
// static int flush_device = 0;
// static tid_t console_task_id;
// int console_is_early = 1;

// static void console_pool_init(void) {
//     int i;
//     INIT_LIST_HEAD(&console_pool_free_list);
//     for (i = 0; i < CONFIG_CONSOLE_POOL_SZ; ++i) {
//         console_pool[i].buffer_offset = 0;
//         console_pool[i].from_pool = 1;
//         INIT_LIST_HEAD(&console_pool[i].list);
//         list_add(&console_pool[i].list, &console_pool_free_list);
//     }
// }

// static struct console_lazy_buffer *console_pool_alloc_nosleep(void) {
//     struct list_head *pos;
//     if (list_empty(&console_pool_free_list))
//         return NULL;

//     pos = console_pool_free_list.next;
//     list_del(pos);
//     return list_entry(pos, struct console_lazy_buffer, list);
// }

// static void console_buffer_free(struct console_lazy_buffer *b) {
//     if (!b)
//         return;
//     if (b->from_pool) {
//         b->buffer_offset = 0;
//         /* push back to pool free list */
//         unsigned long flags;
//         spin_lock_irqsave(&console_lock, flags);
//         list_add(&b->list, &console_pool_free_list);
//         spin_unlock_irqrestore(&console_lock, flags);
//     } else {
//         kfree(b);
//     }
// }

// static void console_print_dev(int type, const char *s, uint32_t count, int do_flush) {
//     /* TODO: respect type */
//     struct console_dev *entry;
//     struct list_head *pos;

//     list_for_each(pos, &consoles_list) {
//         entry = list_entry(pos, struct console_dev, list);

//         if (!entry->write || (console_is_early && !(entry->flags & CONSOLE_EARLY)))
//             continue;

//         entry->write(s, count);

//         if (entry->flush && do_flush && (!console_is_early || type == CONSOLE_PANIC))
//             entry->flush();
//     }
// }

// static void console_rewind_dev(uint32_t count, int clear) {
//     struct console_dev *entry;
//     struct list_head *pos;

//     list_for_each(pos, &consoles_list) {
//         entry = list_entry(pos, struct console_dev, list);

//         if (!entry->rewind || (console_is_early && !(entry->flags & CONSOLE_EARLY)))
//             continue;

//         entry->rewind(count, clear);
//     }
// }

// static void console_clear_dev() {
//     struct console_dev *entry;
//     struct list_head *pos;

//     list_for_each(pos, &consoles_list) {
//         entry = list_entry(pos, struct console_dev, list);

//         if (!entry->clear || (console_is_early && !(entry->flags & CONSOLE_EARLY)))
//             continue;

//         entry->clear();
//     }
// }

// static int console_flush_buffer(void) {
//     struct list_head local_list;
//     struct list_head *pos, *n;
//     struct console_lazy_buffer *entry;
//     int flushed = 0;

//     INIT_LIST_HEAD(&local_list);

//     /* Move any latest buffer into the lazy list, and then steal the whole lazy list */
//     unsigned long flags;
//     spin_lock_irqsave(&console_lock, flags);
//     int should_flush_dev = flush_device || console_is_early;

//     if (console_latest_buffer) {
//         /* move latest to lazy list */
//         list_add_tail(&console_latest_buffer->list, &console_lazy_list);
//         console_latest_buffer = NULL;
//     }

//     /* Move all items from console_lazy_list to local_list (swap) */
//     if (!list_empty(&console_lazy_list)) {
//         local_list.next = console_lazy_list.next;
//         local_list.prev = console_lazy_list.prev;
//         local_list.next->prev = &local_list;
//         local_list.prev->next = &local_list;

//         /* reinit the shared list to empty */
//         INIT_LIST_HEAD(&console_lazy_list);
//         flushed = 1;
//     }

//     spin_unlock_irqrestore(&console_lock, flags);

//     /* Now process the local list OUTSIDE the lock */
//     list_for_each_safe(pos, n, &local_list) {
//         entry = list_entry(pos, struct console_lazy_buffer, list);
//         list_del(pos);
//         if (entry->buffer_offset > 0) {
//             console_print_dev(0, entry->buffer, entry->buffer_offset, should_flush_dev);
//         }
//         /* return buffer to pool or free memory */
//         console_buffer_free(entry);
//     }

//     return flushed;
// }

// static void console_thread(void *arg) {
//     /* Before switching off early con, flush the buffer last time */
//     console_flush_buffer();

//     console_is_early = 0;
//     kprintf("console: early console disabled");

//     while (1) {
//         /* If there's nothing to do, wait a bit to avoid busy spin. */
//         if (!console_flush_buffer())
//             timer_usleep(1000);
//     }
// }

// static int console_write_char(struct device *chardev, const uint8_t *buf, size_t sz) {
//     console_puts(CONSOLE_PRINT, (const char *)buf, sz);
//     return 0;
// }

// static int console_read_char(struct device *chardev, uint8_t *buf, size_t sz) {
//     if (!chardev || !buf || sz != sizeof(struct console_input))
//         return -EINVAL;
//     struct console_dev *dev = chardev->driver_data;

//     spin_lock(&dev->io_lock);
//     if (dev->in_ring.head == dev->in_ring.tail) {
//         spin_unlock(&dev->io_lock);
//         return 0;
//     }
//     struct console_input *in = (struct console_input*)buf;
//     *in = dev->in_ring.in[dev->in_ring.tail];
//     dev->in_ring.tail++;
//     if (dev->in_ring.tail >= sizeof(dev->in_ring.in) / sizeof(struct console_input)) {
//         dev->in_ring.tail = 0;
//     }
//     spin_unlock(&dev->io_lock);
//     return sz;
// }

// int console_write_queue(struct console_dev *dev, struct console_input in) {
//     if (!dev)
//         return -EINVAL;

//     spin_lock(&dev->io_lock);
//     dev->in_ring.in[dev->in_ring.head] = in;
//     if (dev->in_ring.tail > dev->in_ring.head) {
//         dev->in_ring.head++;
//         dev->in_ring.tail++;
//         if (dev->in_ring.tail >= sizeof(dev->in_ring.in) / sizeof(struct console_input)) {
//             dev->in_ring.tail = 0;
//         }
//     }
//     else {
//         dev->in_ring.head++;
//         if (dev->in_ring.head >= sizeof(dev->in_ring.in) / sizeof(struct console_input)) {
//             dev->in_ring.tail++;
//             dev->in_ring.head = 0;
//         }
//     }
//     spin_unlock(&dev->io_lock);
//     return 0;
// }

// static int console_ioctl_char(struct device *chardev, unsigned long req, void *arg) {
//     int ret = 0;
//     unsigned long flags;
//     uint32_t remaining_to_rewind = (uint32_t)(unsigned long)arg;
//     int clear = (req == CONSOLE_IOCTL_REWIND_CLR);
//     struct list_head to_free;
//     struct list_head *pos, *tmp;
//     struct console_lazy_buffer *entry;

//     INIT_LIST_HEAD(&to_free);

//     spin_lock_irqsave(&console_lock, flags);

//     if (req == CONSOLE_IOCTL_CLR) {
//         console_clear_dev();
//     } else if (req == CONSOLE_IOCTL_FLUSH) {
//         console_flush();
//     } else if (req != CONSOLE_IOCTL_REWIND && req != CONSOLE_IOCTL_REWIND_CLR) {
//         while (remaining_to_rewind > 0) {
//             if (!console_latest_buffer) {
//                 if (list_empty(&console_lazy_list)) {
//                     /* We've exhausted all buffered characters.
//                      * The rest must have already been printed to the hardware. */
//                     break;
//                 }
//                 console_latest_buffer = list_entry(console_lazy_list.prev, struct console_lazy_buffer, list);
//                 list_del(&console_latest_buffer->list);
//             }

//             if (console_latest_buffer->buffer_offset >= remaining_to_rewind) {
//                 /* We can satisfy the remaining rewind entirely within this buffer */
//                 console_latest_buffer->buffer_offset -= remaining_to_rewind;

//                 if (clear) {
//                     memset(&console_latest_buffer->buffer[console_latest_buffer->buffer_offset], 0,
//                            remaining_to_rewind);
//                 }
//                 remaining_to_rewind = 0;
//             } else {
//                 /* The rewind consumes this entire buffer; we must step back to the previous one */
//                 remaining_to_rewind -= console_latest_buffer->buffer_offset;
//                 console_latest_buffer->buffer_offset = 0;

//                 if (clear) {
//                     memset(console_latest_buffer->buffer, 0, CONFIG_CONSOLE_BUFF_SZ);
//                 }

//                 list_add(&console_latest_buffer->list, &to_free);
//                 console_latest_buffer = NULL;
//             }
//         }
//     } else
//         ret = -EINVAL;

//     spin_unlock_irqrestore(&console_lock, flags);

//     list_for_each_safe(pos, tmp, &to_free) {
//         entry = list_entry(pos, struct console_lazy_buffer, list);
//         list_del(pos);
//         console_buffer_free(entry);
//     }

//     if (remaining_to_rewind > 0) {
//         console_rewind_dev(remaining_to_rewind, clear);
//     }

//     return ret;
// }

// static int console_flush_char(struct device *chardev) {
//     console_flush();
//     return 0;
// }

// static struct char_ops console_ops = {.write = &console_write_char,
//                                       .read = &console_read_char,
//                                       .ioctl = &console_ioctl_char,
//                                       .flush = &console_flush_char};

// int console_init(void) {
//     int ret;

//     console_pool_init();

//     register_chardev(TTY_DRIVER, &console_ops, NULL, &console_dev);
//     ret = sched_create_thread("console", &console_task_id, &console_thread, NULL, NULL, NULL);
//     return ret;
// }

// int console_register(struct console_dev *dev) {
//     struct console_dev *entry;
//     struct list_head *pos;

//     if (!dev)
//         return -EINVAL;
//     /* Check that we don't already have such device */
//     if (dev->name)
//         list_for_each(pos, &consoles_list) {
//             entry = list_entry(pos, struct console_dev, list);
//             if (strcmp(dev->name, entry->name) == 0)
//                 return -EBUSY;
//         }

//     list_add(&dev->list, &consoles_list);
//     kprintf("console: new device %s registered", dev->name);
//     return 0;
// }

// int console_unregister(struct console_dev *dev) {
//     if (!dev)
//         return -EINVAL;

//     list_del(&dev->list);
//     kprintf("console: device %s unregistered", dev->name);
//     return 0;
// }

// int console_puts(int type, const char *s, uint32_t count) {
//     unsigned long flags;

//     if (console_is_early || type == CONSOLE_PANIC) {
//         console_print_dev(type, s, count, 1);
//         return 0;
//     }

//     /* Enqueue characters into buffers protected by a single lock block */
//     spin_lock_irqsave(&console_lock, flags);

//     while (count > 0) {
//         /* ensure we have a current buffer */
//         if (!console_latest_buffer) {
//             console_latest_buffer = console_pool_alloc_nosleep();
//             if (!console_latest_buffer) {
//                 /* Static pool exhausted. We CANNOT call kmalloc here because it might sleep,
//                  * and we are holding a spinlock (with IRQs disabled).
//                  * Dropping characters is preferable to deadlocking the kernel.
//                  */
//                 break;
//             }
//             console_latest_buffer->buffer_offset = 0;
//         }

//         uint32_t space = CONFIG_CONSOLE_BUFF_SZ - console_latest_buffer->buffer_offset;
//         uint32_t to_copy = (count < space) ? count : space;

//         /* Bulk copy the string into the buffer rather than iterating per-character */
//         memcpy(&console_latest_buffer->buffer[console_latest_buffer->buffer_offset], s, to_copy);

//         console_latest_buffer->buffer_offset += to_copy;
//         s += to_copy;
//         count -= to_copy;

//         /* if current buffer full, move it to lazy list */
//         if (console_latest_buffer->buffer_offset >= CONFIG_CONSOLE_BUFF_SZ) {
//             list_add_tail(&console_latest_buffer->list, &console_lazy_list);
//             console_latest_buffer = NULL;
//         }
//     }

//     spin_unlock_irqrestore(&console_lock, flags);
//     return 0;
// }

// void console_flush(void) {
//     unsigned long flags;
//     spin_lock_irqsave(&console_lock, flags);
//     flush_device = 1;
//     spin_unlock_irqrestore(&console_lock, flags);
// }

// void console_switch_early(void) {
//     console_flush_buffer();
//     console_is_early = 1;
// }

// void console_switch_normal(void) {
//     console_flush_buffer();
//     console_is_early = 0;
// }
