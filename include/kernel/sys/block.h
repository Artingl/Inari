#ifndef _INARI_BLOCK_H
#define _INARI_BLOCK_H

#include <misc/types.h>
#include <misc/list.h>

#include <kernel/sys/driver.h>

#define BLOCK_DEV_NAME_SIZE 16

struct block_device;
struct block_device_group;

struct block_ops
{
    int (*read_blocks)(struct block_device *bdev, uint64_t lba, void *buf, size_t nblocks);
    int (*write_blocks)(struct block_device *bdev, uint64_t lba, const void *buf, size_t nblocks);
    int (*ioctl)(struct block_device *bdev, unsigned long req, void *arg);
};

struct block_device
{
    uint64_t size;
    void *driver_data;

    dev_t dev;
    struct block_device_group *group;
    struct block_ops *ops;

    struct list_head list;
};

struct block_device_group
{
    char name[BLOCK_DEV_NAME_SIZE + 1]; // 1 byte for null

    uint32_t driver;
    uint32_t block_size;
    
    struct list_head block_devices;
};

int blkdev_init();

/* It is *expected* that naming for each block device group is unique */
int register_blkdev_group(uint32_t driver, uint32_t block_size, const char *name);
int unregister_blkdev_group(uint32_t driver);

int register_blkdev(uint32_t driver, struct block_ops *ops, uint64_t size, void *driver_data, dev_t *dev);
int unregister_blkdev(dev_t dev);


/* Fills the `devs` array with bdevs relative to set offset and limit.
 * Returns the amount of found bdevs or 0 if none */
int block_get_refs(dev_t *devs, uint32_t offset, uint32_t limit);

struct block_device *block_get(dev_t dev);

#endif