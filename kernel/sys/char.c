#include <kernel/inari.h>
#include <kernel/sys/char.h>
#include <kernel/sys/vfs.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/errno.h>
#include <kernel/sys/driver.h>
#include <kernel/event.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/format.h>

static struct char_device_group group_entries[DRIVER_TOTAL];

int chardev_init()
{
    memset(&group_entries[0], 0, sizeof(group_entries));
    return 0;
}

int register_chardev_group(uint32_t driver, const char *name)
{
    if (!name || driver >= DRIVER_TOTAL) return -EINVAL;
    struct char_device_group *chardev_group = &group_entries[driver];

    /* Initialize the group */
    chardev_group->driver = driver;
    INIT_LIST_HEAD(&chardev_group->char_devices);
    strcpy(&chardev_group->name[0], name);
    chardev_group->name[CHAR_DEV_NAME_SIZE] = '\0';

    printk("char: new chardev group %16s; driver %u", chardev_group->name, driver);
    return 0;
}

int unregister_chardev_group(uint32_t driver)
{
    printk("char: unimplemented unregister_chardev_group");
    // event_bus_broadcast((event_t){
    //     .type = EVENT_UNLOAD_CHARDEV
    // });
    return -EINVAL;
}

int register_chardev_ops(uint32_t driver, struct char_ops *ops, void *driver_data)
{
    if (!ops || driver >= DRIVER_TOTAL) return -EINVAL;

    uint32_t minor = 0;
    struct char_device *bdev = kmalloc(sizeof(struct char_device));
    struct char_device_group *group = &group_entries[driver];
    struct list_head *pos;
    struct char_device *entry;

    if (!bdev || group->driver == UNNAMED_DRIVER) return -ENOMEM;

    /* Calculate the minor value based on the total count of devices with such driver */
    list_for_each(pos, &group->char_devices) {
        entry = list_entry(pos, struct char_device, list);
        if (DRVID(entry->dev) == driver && minor <= DEVID(entry->dev))
        {
            minor = DEVID(entry->dev) + 1;
        }
    }

    /* Add the char device */
    bdev->driver_data = driver_data;
    bdev->ops = ops;
    bdev->size = size;
    bdev->group = group;
    bdev->dev = MKDEV(driver, minor);

    list_add_tail(&bdev->list, &group->char_devices);
    event_bus_broadcast((event_t){
        .type = EVENT_LOAD_CHARDEV,
        .as = { .dev = bdev->dev }
    });
    printk("char: new dev:%s%u; driver 0x%04x", group->name, minor, driver);
    return 0;
}

int char_get_refs(dev_t *devs, uint32_t offset, uint32_t limit)
{}

struct char_device *char_get(dev_t dev)
{}