#include <kernel/errno.h>
#include <kernel/event.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sys/block.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/vfs.h>

#include <misc/format.h>
#include <misc/list.h>
#include <misc/string.h>

static struct device_group group_entries[DRIVER_TOTAL];

int blkdev_init() {
    memset(&group_entries[0], 0, sizeof(group_entries));
    for (size_t i = 0; i < DRIVER_TOTAL; i++)
        group_entries[i].type = DEV_GRP_BLOCK;
    return 0;
}

int register_blkdev_group(uint32_t driver, uint32_t block_size, const char *name) {
    if (!name || driver >= DRIVER_TOTAL)
        return -EINVAL;
    struct device_group *bdev_group = &group_entries[driver];

    /* Initialize the group */
    bdev_group->driver = driver;
    bdev_group->block_size = block_size;
    INIT_LIST_HEAD(&bdev_group->devices);
    strcpy(&bdev_group->name[0], name);
    bdev_group->name[DEV_NAME_SIZE] = '\0';

    kprintf("block: new bdev group %16s; driver %u", bdev_group->name, driver);
    return 0;
}

int unregister_blkdev_group(uint32_t driver) {
    kprintf("block: unimplemented unregister_blkdev_group");
    return -EINVAL;
}

int unregister_blkdev(dev_t dev) {
    struct device *bdev = block_get(dev);
    if (!bdev)
        return -ENODEV;
    event_bus_broadcast((event_t){.type = EVENT_UNLOAD_BLKDEV, .as = {.dev = bdev->dev}});
    list_del(&bdev->list);
    kfree(bdev);
    return 0;
}

int register_blkdev(uint32_t driver, struct block_ops *ops, uint64_t size, void *driver_data, dev_t *dev) {
    if (!ops || driver >= DRIVER_TOTAL)
        return -EINVAL;

    uint32_t minor = 0;
    struct device *bdev = kmalloc(sizeof(struct device));
    struct device_group *group = &group_entries[driver];
    struct list_head *pos;
    struct device *entry;

    if (!bdev || group->driver == UNNAMED_DRIVER)
        return -ENOMEM;

    /* Calculate the minor value based on the total count of devices with such driver */
    list_for_each(pos, &group->devices) {
        entry = list_entry(pos, struct device, list);
        if (DRVID(entry->dev) == driver && minor <= DEVID(entry->dev)) {
            minor = DEVID(entry->dev) + 1;
        }
    }

    /* Add the block device */
    bdev->driver_data = driver_data;
    bdev->ops = ops;
    bdev->size = size;
    bdev->group = group;
    bdev->dev = MKDEV(driver, minor);

    list_add_tail(&bdev->list, &group->devices);
    kprintf("block: new dev:blk_%s%u; driver 0x%04x", group->name, minor, driver);
    if (dev)
        *dev = bdev->dev;
    event_bus_broadcast((event_t){.type = EVENT_LOAD_BLKDEV, .as = {.dev = bdev->dev}});
    return 0;
}

int block_get_refs(dev_t *devs, uint32_t offset, uint32_t limit) {
    if (!devs)
        return -EINVAL;

    uint32_t count = 0;
    size_t driver;

    for (driver = 0; driver < DRIVER_TOTAL; driver++) {
        struct device_group *group = &group_entries[driver];
        struct list_head *pos;
        struct device *entry;

        if (group->driver == UNNAMED_DRIVER)
            continue;

        list_for_each(pos, &group->devices) {
            entry = list_entry(pos, struct device, list);
            if (offset > 0) {
                offset--;
                continue;
            }

            devs[count++] = entry->dev;
            if (count >= limit)
                goto end;
        }
    }
end:
    return (int)count;
}

struct device *block_get(dev_t dev) {
    uint32_t driver = DRVID(dev);
    if (driver >= DRIVER_TOTAL)
        return NULL;

    struct device_group *group = &group_entries[driver];
    struct list_head *pos;
    struct device *entry;

    if (group->driver == UNNAMED_DRIVER)
        return NULL;

    list_for_each(pos, &group->devices) {
        entry = list_entry(pos, struct device, list);
        if (entry->dev == dev)
            return entry;
    }

    return NULL;
}
