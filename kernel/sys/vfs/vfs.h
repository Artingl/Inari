#pragma once

#include <kernel/include/C/typedefs.h>
#include <kernel/sys/disks/disks.h>

#include <drivers/fs/fat/fat32.h>
#include <drivers/fs/ext2/ext2.h>

#define VFS_DIRECTORY 0x00
#define VFS_FILE 0x01

enum VFS_RESULT
{
    VFS_SUCCESS = 0,
    VFS_INVALID_FS = 1,
    VSF_INVALID_MOUNT_POINT = 2,
};

enum VFS_TYPES
{
    VFS_FAT32 = 0,
    VFS_EXT2 = 1,
};

struct vfs_entry
{
    const char *entry_path;
    uint8_t permissions;
};

struct vfs_directory
{
    size_t offset;
    struct vfs_info *vfs;
};

struct vfs_info
{
    uint8_t filesystem_id;

    struct vfs_entry *entries;
    size_t entries_count;

    char *mount_path;

    union
    {
        struct fat32_info fat32;
        struct ext2_info ext2;
        // ...
    } fs_metadata;
};

struct vfs_info *vfs_get_structure(const char *path);

int vfs_mount_root(struct gendisk *disk);
int vfs_mount(const char *path, struct gendisk *disk);
int vfs_umount(const char *path);
void vfs_append(struct vfs_info vfs);

int vfs_mount_points();

struct vfs_directory *vfs_opendir(const char *path);
void vfs_closedir(struct vfs_directory *directory);

struct vfs_entry *vfs_readdir(struct vfs_directory *directory);
