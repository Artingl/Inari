#ifndef _INARI_CHAR_H
#define _INARI_CHAR_H

#include <misc/types.h>
#include <misc/list.h>

#include <kernel/sys/driver.h>

#define CHAR_DEV_NAME_SIZE 16

struct char_device;
struct char_device_group;

struct char_ops
{
    int (*read)(struct char_device *bdev, uint8_t *buf, size_t sz);
    int (*write)(struct char_device *bdev, const uint8_t *buf, size_t sz);
    int (*ioctl)(struct char_device *bdev, unsigned long req, void *arg);
};

struct char_device
{
    uint64_t size;
    void *driver_data;

    dev_t dev;
    struct char_device_group *group;
    struct char_ops *ops;

    struct list_head list;
};

struct char_device_group
{
    char name[CHAR_DEV_NAME_SIZE + 1]; // 1 byte for null

    uint32_t driver;
    uint32_t char_size;
    
    struct list_head char_devices;
};

int chardev_init();

/* It is *expected* that naming for each char device group is unique */
int register_chardev_group(uint32_t driver, const char *name);
int unregister_chardev_group(uint32_t driver);

int register_chardev_ops(uint32_t driver, struct char_ops *ops, void *driver_data);


/* Fills the `devs` array with bdevs relative to set offset and limit.
 * Returns the amount of found bdevs or 0 if none */
int char_get_refs(dev_t *devs, uint32_t offset, uint32_t limit);

struct char_device *char_get(dev_t dev);

#endif