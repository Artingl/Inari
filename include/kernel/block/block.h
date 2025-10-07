#ifndef _INARI_BLOCK_H
#define _INARI_BLOCK_H

#include <misc/types.h>

struct block_ops
{
    int (*read_blocks)(struct block_device *bdev, uint64_t lba, void *buf, size_t nblocks);
    int (*write_blocks)(struct block_device *bdev, uint64_t lba, const void *buf, size_t nblocks);
};

struct block_device
{
    char name[16];
    uint64_t size;
    uint32_t block_size;
    void *driver_data;
    struct block_ops *ops;
};

extern struct block_device *block_register_device(const char *name,
                                        uint64_t total_size_bytes,
                                        uint32_t block_size,
                                        struct block_ops *ops,
                                        void *driver_data);

extern void block_unregister_device(struct block_device *bdev);

extern struct block_device *block_get(const char *name);


#endif