#include <kernel/inari.h>
#include <kernel/errno.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/interrupts/irq.h>

#include <arch/sys.h>
#include <misc/list.h>

struct irq_handler_node
{
    irq_handler_t handler;
    uint32_t irq;
    void *dev_id;

    struct list_head list;
};

LIST_HEAD(irq_handlers_list);

int irq_request(uint32_t irq, irq_handler_t handler, void *dev_id)
{
    struct irq_handler_node *node = kmalloc(sizeof(*node));
    if (!node) return -ENOMEM;
    node->irq = irq;
    node->dev_id = dev_id;
    node->handler = handler;
    list_add_tail(&node->list, &irq_handlers_list);
    return 0;
}

int irq_free(uint32_t irq, void *dev_id)
{
    struct list_head *pos, *n;
    struct irq_handler_node *entry;

    list_for_each_safe(pos, n, &irq_handlers_list) {
        entry = list_entry(pos, struct irq_handler_node, list);
        list_del(pos);     // unlink
        kfree(entry);      // and kfree
    }

    return 0;
}

void irq_dispatch(struct interrupt_frame frame)
{
    struct list_head *pos;
    struct irq_handler_node *entry;

    list_for_each(pos, &irq_handlers_list) {
        entry = list_entry(pos, struct irq_handler_node, list);
        if (entry && entry->handler && entry->irq == frame.int_no)
            entry->handler(entry->irq, entry->dev_id);
    }
}
