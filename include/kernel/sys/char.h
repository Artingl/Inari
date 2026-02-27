#ifndef _INARI_CHAR_H
#define _INARI_CHAR_H

#include <misc/list.h>
#include <misc/types.h>

#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

int chardev_init();

/* It is *expected* that naming for each char device group is unique */
int register_chardev_group(uint32_t driver, const char *name);
int unregister_chardev_group(uint32_t driver);

int register_chardev(uint32_t driver, struct char_ops *ops, void *driver_data, dev_t *dev);
int unregister_chardev(dev_t dev);

/* Fills the `devs` array with bdevs relative to set offset and limit.
 * Returns the amount of found bdevs or 0 if none */
int char_get_refs(dev_t *devs, uint32_t offset, uint32_t limit);

struct device *char_get(dev_t dev);

#endif