#include <kernel/inari.h>
#include <kernel/block/block.h>
#include <kernel/mm/kmalloc.h>

#include <misc/list.h>
#include <misc/string.h>

struct block_device_item
{
    struct block_device bdev;
    struct list_head list;
};

LIST_HEAD(block_devices);

struct block_device *block_register_device(
    const char *name,
    uint64_t total_size_bytes,
    uint32_t block_size,
    struct block_ops *ops,
    void *driver_data)
{
    if (!name || !ops) return NULL;

    struct block_device_item *item = kmalloc(sizeof(struct block_device_item));
    if (!item) return NULL;
    
    strncpy(&item->bdev.name[0], name, sizeof(item->bdev.name));
    item->bdev.driver_data = driver_data;
    item->bdev.ops = ops;
    item->bdev.block_size = block_size;
    item->bdev.size = total_size_bytes;

    printk("block: registered %16s", item->bdev.name);
    list_add_tail(&item->list, &block_devices);
    return &item->bdev;
}

void block_unregister_device(struct block_device *bdev)
{
    if (!bdev) return;
    struct list_head *pos, *n;
    struct block_device_item *entry;

    list_for_each_safe(pos, n, &block_devices) {
        entry = list_entry(pos, struct block_device_item, list);
        if (strncmp(entry->bdev.name, bdev->name, sizeof(entry->bdev.name))) {
            printk("block: unregistered %16s", entry->bdev.name);
            list_del(pos);
            kfree(entry);
            return;
        }
    }
}

struct block_device *block_get(const char *name)
{
    if (!name) return NULL;
    struct list_head *pos;
    struct block_device_item *entry;

    list_for_each(pos, &block_devices) {
        entry = list_entry(pos, struct block_device_item, list);
        if (strncmp(entry->bdev.name, name, sizeof(entry->bdev.name)))
            return &entry->bdev;
    }
}

