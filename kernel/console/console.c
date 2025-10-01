#include <kernel/inari.h>
#include <kernel/console/console.h>
#include <kernel/console/earlycon.h>
#include <kernel/errno.h>

#include <misc/string.h>

LIST_HEAD(consoles_list);
static int console_is_early = 1;

int console_init(void)
{
    return 0;
}

int console_register(struct console_dev *dev)
{
    if (!dev)
        return -EINVAL;
    list_add(&dev->list, &consoles_list);
}

int console_unregister(struct console_dev *dev)
{
    if (!dev)
        return -EINVAL;
    list_del(&dev->list);
}

int console_printc(int type, const char *s, uint32_t count)
{;
    struct console_dev *entry;
    struct list_head *pos;
    
    list_for_each(pos, &consoles_list) {
        entry = list_entry(pos, struct console_dev, list);
        
        // If we're on early stage, send the data only to earlycon
        if (console_is_early && !(entry->flags & CONSOLE_EARLY))
            continue;

        entry->write(s, count);
    }
}
