#pragma once

#include <kernel/include/C/typedefs.h>
#include <kernel/sys/devfs/devfs.h>

#define SECTOR_SIZE 512

enum
{
    DISK_SUCCESS = 0,
    DISK_IO_ERROR = 1,
};

struct gendisk
{
    uint32_t id;

    struct devfs_node *block_device;
};

int disk_write(
    struct gendisk *disk,
    uint64_t sector,
    uint64_t nsectors,
    uint8_t *buffer);
int disk_read(
    struct gendisk *disk,
    uint64_t sector,
    uint64_t nsectors,
    uint8_t *buffer);

struct gendisk *alloc_disk(struct devfs_node *block_device);
void cleanup_disk(struct gendisk *disk);
