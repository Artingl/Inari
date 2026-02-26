#include <kernel/inari.h>
#include <kernel/interrupts/irq.h>
#include <kernel/console/console.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/console/earlycon.h>
#include <kernel/proc/sched.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/errno.h>
#include <kernel/timer.h>

#include <misc/string.h>
#include <misc/list.h>
#include <misc/types.h>
#include <kernel/sync/spinlock.h>

struct console_lazy_buffer
{
    char buffer[CONFIG_CONSOLE_BUFF_SZ];
    uint32_t buffer_offset;
    struct list_head list;
    uint8_t from_pool; /* 1 if from static pool, 0 if from kmalloc */
};

LIST_HEAD(consoles_list);
LIST_HEAD(console_lazy_list); /* buffers awaiting printing */

static struct console_lazy_buffer *console_latest_buffer = (struct console_lazy_buffer *)NULL;

static spinlock_t console_lock = {0};

static struct console_lazy_buffer console_pool[CONFIG_CONSOLE_POOL_SZ];
static LIST_HEAD(console_pool_free_list);

static int flush_device = 0;
static tid_t console_task_id;
int console_is_early = 1;

static void console_pool_init(void)
{
    int i;
    INIT_LIST_HEAD(&console_pool_free_list);
    for (i = 0; i < CONFIG_CONSOLE_POOL_SZ; ++i) {
        console_pool[i].buffer_offset = 0;
        console_pool[i].from_pool = 1;
        INIT_LIST_HEAD(&console_pool[i].list);
        list_add(&console_pool[i].list, &console_pool_free_list);
    }
}

static struct console_lazy_buffer *console_pool_alloc_nosleep(void)
{
    struct list_head *pos;
    if (list_empty(&console_pool_free_list))
        return NULL;

    pos = console_pool_free_list.next;
    list_del(pos);
    return list_entry(pos, struct console_lazy_buffer, list);
}

static void console_buffer_free(struct console_lazy_buffer *b)
{
    if (!b) return;
    if (b->from_pool) {
        b->buffer_offset = 0;
        /* push back to pool free list */
        unsigned long flags;
        spin_lock_irqsave(&console_lock, flags);
        list_add(&b->list, &console_pool_free_list);
        spin_unlock_irqrestore(&console_lock, flags);
    } else {
        kfree(b);
    }
}

static void console_print_dev(int type, const char *s, uint32_t count, int do_flush)
{
    /* TODO: respect type */
    struct console_dev *entry;
    struct list_head *pos;

    list_for_each(pos, &consoles_list) {
        entry = list_entry(pos, struct console_dev, list);

        if (!entry->write || (console_is_early && !(entry->flags & CONSOLE_EARLY)))
            continue;

        entry->write(s, count);

        if (entry->flush && do_flush && !console_is_early)
            entry->flush();
    }
}

static void console_rewind_dev(uint32_t count, int clear)
{
    struct console_dev *entry;
    struct list_head *pos;

    list_for_each(pos, &consoles_list) {
        entry = list_entry(pos, struct console_dev, list);

        if (!entry->rewind || (console_is_early && !(entry->flags & CONSOLE_EARLY)))
            continue;

        entry->rewind(count, clear);
    }
}

static void console_clear_dev()
{
    struct console_dev *entry;
    struct list_head *pos;

    list_for_each(pos, &consoles_list) {
        entry = list_entry(pos, struct console_dev, list);

        if (!entry->clear || (console_is_early && !(entry->flags & CONSOLE_EARLY)))
            continue;

        entry->clear();
    }
}

static int console_flush_buffer(void)
{
    struct list_head local_list;
    struct list_head *pos, *n;
    struct console_lazy_buffer *entry;
    int flushed = 0;

    INIT_LIST_HEAD(&local_list);
    
    /* Move any latest buffer into the lazy list, and then steal the whole lazy list */
    unsigned long flags;
    spin_lock_irqsave(&console_lock, flags);
    int should_flush_dev = flush_device || console_is_early;

    if (console_latest_buffer) {
        /* move latest to lazy list */
        list_add_tail(&console_latest_buffer->list, &console_lazy_list);
        console_latest_buffer = NULL;
    }

    /* Move all items from console_lazy_list to local_list (swap) */
    if (!list_empty(&console_lazy_list)) {
        local_list.next = console_lazy_list.next;
        local_list.prev = console_lazy_list.prev;
        local_list.next->prev = &local_list;
        local_list.prev->next = &local_list;

        /* reinit the shared list to empty */
        INIT_LIST_HEAD(&console_lazy_list);
        flushed = 1;
    }

    spin_unlock_irqrestore(&console_lock, flags);

    /* Now process the local list OUTSIDE the lock */
    list_for_each_safe(pos, n, &local_list) {
        entry = list_entry(pos, struct console_lazy_buffer, list);
        list_del(pos);
        if (entry->buffer_offset > 0) {
            console_print_dev(0, entry->buffer, entry->buffer_offset, should_flush_dev);
        }
        /* return buffer to pool or free memory */
        console_buffer_free(entry);
    }

    return flushed;
}

static void console_thread(void* arg)
{
    /* Before switching off early con, flush the buffer last time */
    console_flush_buffer();

    console_is_early = 0;
    kprintf("console: early console disabled");

    while (1)
    {
        /* If there's nothing to do, wait a bit to avoid busy spin. */
        if (!console_flush_buffer())
            usleep(1000);
    }
}

static dev_t console_dev = 0;
static int console_write_char(struct device *chardev, const uint8_t *buf, size_t sz)
{
    console_puts(CONSOLE_PRINT, (const char *)buf, sz);
    return 0;
}

static int console_read_char(struct device *chardev, uint8_t *buf, size_t sz)
{
    return -1;
}

static int console_ioctl_char(struct device *chardev, unsigned long req, void *arg)
{
    int ret = 0;
    unsigned long flags;
    uint32_t remaining_to_rewind = (uint32_t)(unsigned long)arg;
    int clear = (req == CONSOLE_IOCTL_REWIND_CLR);
    struct list_head to_free;
    struct list_head *pos, *tmp;
    struct console_lazy_buffer *entry;

    INIT_LIST_HEAD(&to_free);

    spin_lock_irqsave(&console_lock, flags);

    if (req == CONSOLE_IOCTL_CLR)
    {
        console_clear_dev();
    }
    else if (req == CONSOLE_IOCTL_FLUSH)
    {
        console_flush();
    }
    else if (req != CONSOLE_IOCTL_REWIND && req != CONSOLE_IOCTL_REWIND_CLR)
    {
        while (remaining_to_rewind > 0)
        {
            if (!console_latest_buffer) {
                if (list_empty(&console_lazy_list)) {
                    /* We've exhausted all buffered characters. 
                    * The rest must have already been printed to the hardware. */
                    break; 
                }
                console_latest_buffer = list_entry(console_lazy_list.prev, struct console_lazy_buffer, list);
                list_del(&console_latest_buffer->list);
            }

            if (console_latest_buffer->buffer_offset >= remaining_to_rewind) {
                /* We can satisfy the remaining rewind entirely within this buffer */
                console_latest_buffer->buffer_offset -= remaining_to_rewind;
                
                if (clear) {
                    memset(&console_latest_buffer->buffer[console_latest_buffer->buffer_offset], 0, remaining_to_rewind);
                }
                remaining_to_rewind = 0;
            } else {
                /* The rewind consumes this entire buffer; we must step back to the previous one */
                remaining_to_rewind -= console_latest_buffer->buffer_offset;
                console_latest_buffer->buffer_offset = 0;
                
                if (clear) {
                    memset(console_latest_buffer->buffer, 0, CONFIG_CONSOLE_BUFF_SZ);
                }

                list_add(&console_latest_buffer->list, &to_free);
                console_latest_buffer = NULL;
            }
        }
    }
    else ret = -EINVAL;

    spin_unlock_irqrestore(&console_lock, flags);

    list_for_each_safe(pos, tmp, &to_free) {
        entry = list_entry(pos, struct console_lazy_buffer, list);
        list_del(pos);
        console_buffer_free(entry);
    }

    if (remaining_to_rewind > 0) {
        console_rewind_dev(remaining_to_rewind, clear);
    }

    return ret;
}

static int console_flush_char(struct device *chardev)
{
    console_flush();
    return 0;
}

static struct char_ops console_ops = {
    .write = &console_write_char,
    .read = &console_read_char,
    .ioctl = &console_ioctl_char,
    .flush = &console_flush_char
};

int console_init(void)
{
    int ret;

    console_pool_init();

    register_chardev(TTY_DRIVER, &console_ops, NULL, &console_dev);
    ret = sched_create_thread(&console_task_id, &console_thread, NULL, NULL, NULL);
    return ret;
}

int console_register(struct console_dev *dev)
{
    struct console_dev *entry;
    struct list_head *pos;

    if (!dev)
        return -EINVAL;
    /* Check that we don't already have such device */
    if (dev->name)
        list_for_each(pos, &consoles_list) {
            entry = list_entry(pos, struct console_dev, list);
            if (strcmp(dev->name, entry->name) == 0)
                return -EBUSY;
        }

    list_add(&dev->list, &consoles_list);
    kprintf("console: new device %s registered", dev->name);
    return 0;
}

int console_unregister(struct console_dev *dev)
{
    if (!dev)
        return -EINVAL;
    
    list_del(&dev->list);
    kprintf("console: device %s unregistered", dev->name);
    return 0;
}

int console_puts(int type, const char *s, uint32_t count)
{
    unsigned long flags;

    if (console_is_early || type == CONSOLE_PANIC)
    {
        console_print_dev(type, s, count, 1);
        return 0;
    }

    /* Enqueue characters into buffers protected by a single lock block */
    spin_lock_irqsave(&console_lock, flags);

    while (count > 0)
    {
        /* ensure we have a current buffer */
        if (!console_latest_buffer) {
            console_latest_buffer = console_pool_alloc_nosleep();
            if (!console_latest_buffer) {
                /* Static pool exhausted. We CANNOT call kmalloc here because it might sleep,
                 * and we are holding a spinlock (with IRQs disabled).
                 * Dropping characters is preferable to deadlocking the kernel.
                 */
                break;
            }
            console_latest_buffer->buffer_offset = 0;
        }

        uint32_t space = CONFIG_CONSOLE_BUFF_SZ - console_latest_buffer->buffer_offset;
        uint32_t to_copy = (count < space) ? count : space;

        /* Bulk copy the string into the buffer rather than iterating per-character */
        memcpy(&console_latest_buffer->buffer[console_latest_buffer->buffer_offset], s, to_copy);
        
        console_latest_buffer->buffer_offset += to_copy;
        s += to_copy;
        count -= to_copy;

        /* if current buffer full, move it to lazy list */
        if (console_latest_buffer->buffer_offset >= CONFIG_CONSOLE_BUFF_SZ)
        {
            list_add_tail(&console_latest_buffer->list, &console_lazy_list);
            console_latest_buffer = NULL;
        }
    }

    spin_unlock_irqrestore(&console_lock, flags);
    return 0;
}

void console_flush(void)
{
    unsigned long flags;
    spin_lock_irqsave(&console_lock, flags);
    flush_device = 1;
    spin_unlock_irqrestore(&console_lock, flags);
}

void console_switch_early(void)
{
    console_flush_buffer();
    console_is_early = 1;
}

void console_switch_normal(void)
{
    console_flush_buffer();
    console_is_early = 0;
}