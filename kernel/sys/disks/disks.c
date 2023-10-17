#include <kernel/kernel.h>

#include <kernel/sys/devfs/devfs.h>
#include <kernel/sys/disks/disks.h>

struct gendisk *alloc_disk(struct devfs_node *block_device)
{
    struct gendisk *disk = kmalloc(sizeof(struct gendisk));
    disk->block_device = block_device;
    return disk;
}

int disk_write(struct gendisk *disk,
                uint64_t sector,
                uint64_t nsectors,
                uint8_t *buffer)
{
    if (devfs_block_write(disk->block_device, sector, nsectors, buffer) == DEVFS_SUCCESS)
    {
        return DISK_SUCCESS;
    }

    return DISK_IO_ERROR;
}

int disk_read(struct gendisk *disk,
               uint64_t sector,
               uint64_t nsectors,
               uint8_t *buffer)
{
    if (devfs_block_read(disk->block_device, sector, nsectors, buffer) == DEVFS_SUCCESS)
    {
        return DISK_SUCCESS;
    }

    return DISK_IO_ERROR;
}

void cleanup_disk(struct gendisk *disk)
{
    kfree(disk);
}
