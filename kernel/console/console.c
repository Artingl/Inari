#include <kernel/inari.h>
#include <kernel/irq/irq.h>
#include <kernel/console/console.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/console/earlycon.h>
#include <kernel/lock/spinlock.h>
#include <kernel/errno.h>

#include <misc/string.h>

#define CONSOLE_BUFFER_SIZE 128

struct console_lazy_buffer
{
    char buffer[CONSOLE_BUFFER_SIZE];
    uint32_t buffer_offset;
    struct list_head list;
};

LIST_HEAD(consoles_list);
LIST_HEAD(console_lazy_list);

static struct console_lazy_buffer *console_latest_buffer = (struct console_lazy_buffer *)NULL;
static spinlock_t console_lock;
static int console_is_early = 1;

static void __console_print_dev(int type, const char *s, uint32_t count)
{
    // TODO: respect the value of type

    struct console_dev *entry;
    struct list_head *pos;
    
    list_for_each(pos, &consoles_list) {
        entry = list_entry(pos, struct console_dev, list);
        
        // If we're on early stage, send the data only to earlycon
        if (!entry->write || (console_is_early && !(entry->flags & CONSOLE_EARLY)))
            continue;

        entry->write(s, count);
    }
}

static int __console_irq_handler(uint32_t irq, void *dev_id)
{
    size_t i;
    struct list_head *pos, *n;
    struct console_lazy_buffer *entry;

    if (spinlock_test(&console_lock) == 0)
    {
        // Check if we have unsaved buffer
        if (console_latest_buffer)
            list_add_tail(&console_latest_buffer->list, &console_lazy_list);
        console_latest_buffer = (struct console_lazy_buffer*)NULL;

        // Print the whole buffer to the screen
        list_for_each_safe(pos, n, &console_lazy_list) {
            entry = list_entry(pos, struct console_lazy_buffer, list);
            if (entry->buffer_offset > 0)
                __console_print_dev(0, &entry->buffer[0], entry->buffer_offset);
            list_del(pos);     // unlink
            kfree(entry);      // and kfree
        }

        spinlock_release(&console_lock);
    }

    return IRQ_HANDLED;
}

int console_init(void)
{
    // TODO: using irq for that is an awful idea. Better to use scheduler when available
    irq_request(IRQ_TIMER_INTERRUPT, &__console_irq_handler, NULL);
    console_is_early = 0;
    return 0;
}

int console_register(struct console_dev *dev)
{
    if (!dev)
        return -EINVAL;
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

int console_printc(int type, const char *s, uint32_t count)
{
    spinlock_acquire(&console_lock);

    // If early console is used, print directly
    if (console_is_early)
    {
        __console_print_dev(type, s, count);
    }
    // Add the text to the lazy list
    else {
        while (count-- > 0)
        {
            if (!console_latest_buffer || console_latest_buffer->buffer_offset + 1 >= CONSOLE_BUFFER_SIZE)
            {
                if (console_latest_buffer)
                    list_add_tail(&console_latest_buffer->list, &console_lazy_list);
                console_latest_buffer = kmalloc(sizeof(*console_latest_buffer));
                console_latest_buffer->buffer_offset = 0;
            }

            // TODO: support for saving the type of print
            console_latest_buffer->buffer[console_latest_buffer->buffer_offset++] = *s++;
        }
    }

    spinlock_release(&console_lock);

    return 0;
}
