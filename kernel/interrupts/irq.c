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
    void *driver_data;

    struct list_head list;
};

LIST_HEAD(irq_handlers_list);

int irq_request(uint32_t irq, irq_handler_t handler, void *driver_data)
{
    struct irq_handler_node *node = kmalloc(sizeof(*node));
    if (!node) return -ENOMEM;
    node->irq = irq;
    node->driver_data = driver_data;
    node->handler = handler;
    list_add_tail(&node->list, &irq_handlers_list);
    return 0;
}

int irq_free(uint32_t irq, irq_handler_t handler)
{
    struct list_head *pos, *n;
    struct irq_handler_node *entry;

    list_for_each_safe(pos, n, &irq_handlers_list) {
        entry = list_entry(pos, struct irq_handler_node, list);
        if (entry->irq == irq && entry->handler == handler) {
            list_del(pos);
            kfree(entry);
            return 0;
        }
    }

    return -1;
}

void irq_dispatch(struct interrupt_frame frame)
{
    struct list_head *pos;
    struct irq_handler_node *entry;

    list_for_each(pos, &irq_handlers_list) {
        entry = list_entry(pos, struct irq_handler_node, list);
        if (entry && entry->handler && entry->irq == frame.int_no)
            entry->handler(entry->irq, entry->driver_data);
    }
}
