#ifndef _INARI_BLOCK_H
#define _INARI_BLOCK_H

#include <misc/list.h>
#include <misc/types.h>

#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

int blkdev_init();

/* It is *expected* that naming for each block device group is unique */
int register_blkdev_group(uint32_t driver, uint32_t block_size, const char *name);
int unregister_blkdev_group(uint32_t driver);

int register_blkdev(uint32_t driver, struct block_ops *ops, uint64_t size, void *driver_data, dev_t *dev);
int unregister_blkdev(dev_t dev);

/* Fills the `devs` array with bdevs relative to set offset and limit.
 * Returns the amount of found bdevs or 0 if none */
int block_get_refs(dev_t *devs, uint32_t offset, uint32_t limit);

struct device *block_get(dev_t dev);

#endif