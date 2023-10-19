#pragma once

#include <kernel/include/C/typedefs.h>
#include <kernel/sys/disks/disks.h>

enum
{
    VFS_SUCCESS = 0,
};

int vfs_init();
int vfs_mount_root(struct gendisk *disk);
