#include <kernel/inari.h>
#include <kernel/interrupts/irq.h>
#include <kernel/console/console.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/console/earlycon.h>
#include <kernel/sched/sched.h>
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

/* Spinlock used for tiny critical sections; irqsave because writers may be in IRQ */
static spinlock_t console_lock;

/* Static pool so IRQ context never needs to kmalloc (kmalloc may sleep) */
static struct console_lazy_buffer console_pool[CONFIG_CONSOLE_POOL_SZ];
static LIST_HEAD(console_pool_free_list);

static int console_is_early = 1;
static tid_t console_task_id;

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

static void console_print_dev(int type, const char *s, uint32_t count)
{
    /* TODO: respect type */
    struct console_dev *entry;
    struct list_head *pos;

    list_for_each(pos, &consoles_list) {
        entry = list_entry(pos, struct console_dev, list);

        if (!entry->write || (console_is_early && !(entry->flags & CONSOLE_EARLY)))
            continue;

        entry->write(s, count);
    }
}

/*
 * console_main_task:
 *   take ownership of console_lazy_list under lock, then release lock and print.
 *   This ensures we never hold the spinlock while calling console_print_dev (which may sleep).
 */
static void console_thread(void)
{
    earlycom_cleanup();
    console_is_early = 0;
    printk("console: early console disabled");

    while (1)
    {
        struct list_head local_list;
        struct list_head *pos, *n;
        struct console_lazy_buffer *entry;

        INIT_LIST_HEAD(&local_list);
        
        /* Move any latest buffer into the lazy list, and then steal the whole lazy list */
        unsigned long flags;
        spin_lock_irqsave(&console_lock, flags);

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
        }

        spin_unlock_irqrestore(&console_lock, flags);

        /* Now process the local list OUTSIDE the lock */
        list_for_each_safe(pos, n, &local_list) {
            entry = list_entry(pos, struct console_lazy_buffer, list);
            list_del(pos);
            if (entry->buffer_offset > 0) {
                console_print_dev(0, entry->buffer, entry->buffer_offset);
            }
            /* return buffer to pool or free memory */
            console_buffer_free(entry);
        }

        /* If there's nothing to do, wait a bit to avoid busy spin. */
        if (list_empty(&local_list)) {
            usleep(100000);
        }
    }
}

int console_init(void)
{
    int ret;

    /* init spinlock and pool */
    spinlock_init(&console_lock);
    console_pool_init();

    ret = sched_add_task(&console_task_id, &console_thread);
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
    return 0;
}

int console_unregister(struct console_dev *dev)
{
    if (!dev)
        return -EINVAL;
    list_del(&dev->list);
    return 0;
}

/*
 * console_printc:
 *   - If early or panic, print directly (as before).
 *   - Otherwise append chars to a buffer.
 *   - Allocation attempts to use the static pool (no sleep). If pool empty,
 *     we try to kmalloc (may sleep); however, since we cannot detect IRQ
 *     context portably here, we conservatively *prefer pool* and drop chars
 *     when pool is exhausted to stay safe for IRQ contexts.
 */
int console_printc(int type, const char *s, uint32_t count)
{
    if (console_is_early || type == CONSOLE_PANIC)
    {
        console_print_dev(type, s, count);
        return 0;
    }

    /* Enqueue characters into buffers protected by spinlock (irqsave) */
    while (count-- > 0)
    {
        unsigned long flags;
        spin_lock_irqsave(&console_lock, flags);

        /* ensure we have a current buffer */
        if (!console_latest_buffer) {
            /* try pool first (non-sleeping) */
            console_latest_buffer = console_pool_alloc_nosleep();
            if (console_latest_buffer) {
                console_latest_buffer->buffer_offset = 0;
                /* from_pool already set by pool init */
            } else {
                /* pool exhausted: fallback to kmalloc (may sleep). If kmalloc fails immediately,
                   we drop the character to avoid deadlocks in IRQ context. */
                console_latest_buffer = kmalloc(sizeof(*console_latest_buffer));
                if (console_latest_buffer) {
                    console_latest_buffer->buffer_offset = 0;
                    console_latest_buffer->from_pool = 0;
                    INIT_LIST_HEAD(&console_latest_buffer->list);
                } else {
                    /* can't allocate - drop the char */
                    spin_unlock_irqrestore(&console_lock, flags);
                    s++;
                    continue;
                }
            }
        }

        /* if current buffer full, move it to lazy list and allocate new one */
        if (console_latest_buffer->buffer_offset + 1 >= CONFIG_CONSOLE_BUFF_SZ)
        {
            list_add_tail(&console_latest_buffer->list, &console_lazy_list);
            console_latest_buffer = NULL;

            /* allocate new buffer for the next char */
            console_latest_buffer = console_pool_alloc_nosleep();
            if (console_latest_buffer) {
                console_latest_buffer->buffer_offset = 0;
            } else {
                console_latest_buffer = kmalloc(sizeof(*console_latest_buffer));
                if (console_latest_buffer) {
                    console_latest_buffer->buffer_offset = 0;
                    console_latest_buffer->from_pool = 0;
                    INIT_LIST_HEAD(&console_latest_buffer->list);
                } else {
                    /* pool and kmalloc both failed; drop char */
                    spin_unlock_irqrestore(&console_lock, flags);
                    s++;
                    continue;
                }
            }
        }

        /* append char */
        console_latest_buffer->buffer[console_latest_buffer->buffer_offset++] = *s++;

        spin_unlock_irqrestore(&console_lock, flags);
    }
    return 0;
}
