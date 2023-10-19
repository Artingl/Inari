#include <kernel/kernel.h>

#include <kernel/sys/vfs/vfs.h>

#include <drivers/fs/fs.h>

int vfs_init()
{
    return VFS_SUCCESS;
}

int vfs_mount_root(struct gendisk *disk)
{
    // struct fs;

    // if (fat32_make(block_device))
    // {
    // }

    return VFS_SUCCESS;
}
