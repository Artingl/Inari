#include <kernel/errno.h>
#include <kernel/event.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/vfs.h>

#include <misc/format.h>
#include <misc/list.h>
#include <misc/string.h>

static struct device_group group_entries[DRIVER_TOTAL];

int chardev_init() {
    memset(&group_entries[0], 0, sizeof(group_entries));
    for (size_t i = 0; i < DRIVER_TOTAL; i++)
        group_entries[i].type = DEV_GRP_CHAR;
    return 0;
}

int register_chardev_group(uint32_t driver, const char *name) {
    if (!name || driver >= DRIVER_TOTAL)
        return -EINVAL;
    struct device_group *chardev_group = &group_entries[driver];

    /* Initialize the group */
    chardev_group->driver = driver;
    INIT_LIST_HEAD(&chardev_group->devices);
    strcpy(&chardev_group->name[0], name);
    chardev_group->name[DEV_NAME_SIZE] = '\0';

    kprintf("char: new chardev group %16s; driver %u", chardev_group->name, driver);
    return 0;
}

int unregister_chardev_group(uint32_t driver) {
    kprintf("char: unimplemented unregister_chardev_group");
    return -EINVAL;
}

int unregister_chardev(dev_t dev) {
    struct device *chardev = char_get(dev);
    if (!chardev)
        return -ENODEV;
    event_bus_broadcast((event_t){.type = EVENT_UNLOAD_CHARDEV, .as = {.dev = chardev->dev}});
    list_del(&chardev->list);
    kfree(chardev);
    return 0;
}

int register_chardev(uint32_t driver, struct char_ops *ops, void *driver_data, dev_t *dev) {
    if (!ops || driver >= DRIVER_TOTAL)
        return -EINVAL;

    uint32_t minor = 0;
    struct device *chardev = kmalloc(sizeof(struct device));
    struct device_group *group = &group_entries[driver];
    struct list_head *pos;
    struct device *entry;

    if (!chardev || group->driver == UNNAMED_DRIVER)
        return -ENOMEM;

    /* Calculate the minor value based on the total count of devices with such driver */
    list_for_each(pos, &group->devices) {
        entry = list_entry(pos, struct device, list);
        if (DRVID(entry->dev) == driver && minor <= DEVID(entry->dev)) {
            minor = DEVID(entry->dev) + 1;
        }
    }

    /* Add the char device */
    chardev->driver_data = driver_data;
    chardev->ops = ops;
    chardev->group = group;
    chardev->dev = MKDEV(driver, minor);

    list_add_tail(&chardev->list, &group->devices);
    kprintf("char: new dev:char_%s%u; driver 0x%04x", group->name, minor, driver);
    if (dev)
        *dev = chardev->dev;
    event_bus_broadcast((event_t){.type = EVENT_LOAD_CHARDEV, .as = {.dev = chardev->dev}});
    return 0;
}

int char_get_refs(dev_t *devs, uint32_t offset, uint32_t limit) {
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

struct device *char_get(dev_t dev) {
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
