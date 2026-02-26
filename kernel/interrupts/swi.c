#include <kernel/inari.h>
#include <kernel/errno.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/interrupts/swi.h>

#include <arch/sys.h>
#include <misc/list.h>

struct swi_handler_node
{
    swi_handler_t handler;
    uint32_t swi;
    void *dev_id;

    struct list_head list;
};

LIST_HEAD(swi_handlers_list);

int swi_request(uint32_t swi, swi_handler_t handler, void *dev_id)
{
    struct swi_handler_node *node = kmalloc(sizeof(*node));
    if (!node) return -ENOMEM;
    node->swi = swi;
    node->dev_id = dev_id;
    node->handler = handler;
    list_add_tail(&node->list, &swi_handlers_list);
    return 0;
}

int swi_free(uint32_t swi, swi_handler_t handler)
{
    struct list_head *pos, *n;
    struct swi_handler_node *entry;

    list_for_each_safe(pos, n, &swi_handlers_list) {
        entry = list_entry(pos, struct swi_handler_node, list);
        if (entry->swi == swi && entry->handler == handler) {
            list_del(pos);
            kfree(entry);
            return 0;
        }
    }

    return -1;
}

void swi_dispatch(struct interrupt_frame frame)
{
    struct list_head *pos;
    struct swi_handler_node *entry;

    list_for_each(pos, &swi_handlers_list) {
        entry = list_entry(pos, struct swi_handler_node, list);
        if (entry && entry->handler && entry->swi == frame.int_no)
            entry->handler(entry->swi, entry->dev_id);
    }
}
