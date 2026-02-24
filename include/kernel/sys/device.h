#ifndef _INARI_DEVICE_H
#define _INARI_DEVICE_H

#include <misc/list.h>

#define DEV_NAME_SIZE 16

#define DEV_GRP_BLOCK     0
#define DEV_GRP_CHAR      1

struct device;
struct device_group;

struct char_ops
{
    int (*read)(struct device *chardev, uint8_t *buf, size_t sz);
    int (*write)(struct device *chardev, const uint8_t *buf, size_t sz);
    int (*ioctl)(struct device *chardev, unsigned long req, void *arg);
};

struct block_ops
{
    int (*read_blocks)(struct device *bdev, uint64_t lba, void *buf, size_t nblocks);
    int (*write_blocks)(struct device *bdev, uint64_t lba, const void *buf, size_t nblocks);
    int (*ioctl)(struct device *bdev, unsigned long req, void *arg);
};

struct device
{
    uint64_t size;
    void *driver_data;

    dev_t dev;
    struct device_group *group;
    void *ops;

    struct list_head list;
};

struct device_group
{
    uint8_t type;                 // DEV_GRP_BLOCK, DEV_GRP_CHAR
    char name[DEV_NAME_SIZE + 1]; // 1 byte for null

    uint32_t driver;
    uint32_t block_size;
    
    struct list_head devices;
};

#endif