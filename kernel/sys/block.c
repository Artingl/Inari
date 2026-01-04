#include <kernel/inari.h>
#include <kernel/sys/block.h>
#include <kernel/sys/vfs.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/errno.h>
#include <kernel/sys/driver.h>
#include <kernel/event.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/format.h>

static struct block_device_group group_entries[DRIVER_TOTAL];

int blkdev_init()
{
    memset(&group_entries[0], 0, sizeof(group_entries));
    return 0;
}

int register_blkdev_group(uint32_t driver, uint32_t block_size, const char *name)
{
    if (!name || driver >= DRIVER_TOTAL) return -EINVAL;
    struct block_device_group *bdev_group = &group_entries[driver];

    /* Initialize the group */
    bdev_group->driver = driver;
    bdev_group->block_size = block_size;
    INIT_LIST_HEAD(&bdev_group->block_devices);
    strcpy(&bdev_group->name[0], name);
    bdev_group->name[BLOCK_DEV_NAME_SIZE] = '\0';

    printk("block: %s registered as /%16s", driver_namings[driver], bdev_group->name);
    return 0;
}

int unregister_blkdev_group(uint32_t driver)
{
    printk("block: unimplemented unregister_blkdev_group");
    // event_bus_broadcast((event_t){
    //     .type = EVENT_UNLOAD_BLKDEV
    // });
    return -EINVAL;
}

int register_blkdev_ops(dev_t dev, struct block_ops *ops, uint64_t size, void *driver_data)
{
    uint32_t driver = DRVID(dev), minor = DEVID(dev);

    if (!ops || driver >= DRIVER_TOTAL) return -EINVAL;

    struct block_device *bdev = kmalloc(sizeof(struct block_device));
    struct block_device_group *group = &group_entries[driver];

    if (!bdev || group->driver == UNNAMED_DRIVER) return -ENOMEM;

    /* Add the block device */
    bdev->driver_data = driver_data;
    bdev->ops = ops;
    bdev->size = size;
    bdev->group = group;
    bdev->dev = dev;

    list_add_tail(&bdev->list, &group->block_devices);
    event_bus_broadcast((event_t){
        .type = EVENT_LOAD_BLKDEV,
        .as = { .blkdev = dev }
    });
    printk("block: registered new %s device %u", driver_namings[driver], minor);
    return 0;
}

int block_get_refs(dev_t *devs, uint32_t offset, uint32_t limit)
{
    if (!devs) return -EINVAL;

    int count = 0;
    size_t driver;

    for (driver = 0; driver < DRIVER_TOTAL; driver++)
    {
        struct block_device_group *group = &group_entries[driver];
        struct list_head *pos;
        struct block_device *entry;

        if (group->driver == UNNAMED_DRIVER) continue;

        list_for_each(pos, &group->block_devices) {
            entry = list_entry(pos, struct block_device, list);
            if (offset > 0)
            {
                offset--;
                continue;
            }

            devs[count++] = entry->dev;
            if (count >= limit) goto end;
        }
    }
end:
    return count;
}

struct block_device *block_get(dev_t dev)
{
    uint32_t driver = DRVID(dev), minor = DEVID(dev);
    if (driver >= DRIVER_TOTAL) return NULL;

    struct block_device_group *group = &group_entries[driver];
    struct list_head *pos;
    struct block_device *entry;

    if (group->driver == UNNAMED_DRIVER) return NULL;

    list_for_each(pos, &group->block_devices) {
        entry = list_entry(pos, struct block_device, list);
        if (entry->dev == dev)
            return entry;
    }

    return NULL;
}

