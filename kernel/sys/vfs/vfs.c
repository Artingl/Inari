#include <kernel/kernel.h>
#include <kernel/include/C/string.h>
#include <kernel/sys/vfs/vfs.h>

struct vfs_info **vfs_list;
size_t vfs_list_count = 0;

void vfs_append(struct vfs_info vfs)
{
    size_t list_idx = vfs_list_count;
    struct vfs_info *vfs_allocated = kmalloc(sizeof(struct vfs_info));

    vfs_list_count++;
    vfs_list = krealloc(vfs_list, vfs_list_count * sizeof(struct vfs_info *));
    vfs_list[list_idx] = vfs_allocated;
}

struct vfs_info *vfs_get_structure(const char *path)
{
    size_t i;
    for (i = 0; i < vfs_list_count; i++)
    {
        if (strcmp(vfs_list[i]->mount_path, path) == 0)
        {
            return vfs_list[i];
        }
    }

    return NULL;
}

int vfs_mount_points()
{
    return vfs_list_count;
}

int vfs_umount(const char *path)
{
    // TODO: kfree vfs.path, resize the vfs array
    struct vfs_info *vfs = vfs_get_structure(path);
    struct vfs_info **new_vfs_list;
    size_t i, j = 0;

    if (vfs == NULL)
    {
        return VSF_INVALID_MOUNT_POINT;
    }

    // rearrange the vfs list
    new_vfs_list = kcalloc(vfs_list_count - 1, sizeof(struct vfs_info *));
    for (i = 0; i < vfs_list_count; i++)
    {
        if (strcmp(vfs_list[i]->mount_path, path) != 0)
        {
            new_vfs_list[j++] = vfs_list[i];
        }
    }

    // cleanup the fs
    switch (vfs->filesystem_id)
    {
    case VFS_FAT32:
        fat32_cleanup(&vfs->fs_metadata.fat32);
        break;
    case VFS_EXT2:
        ext2_cleanup(&vfs->fs_metadata.ext2);
        break;

    default:
        panic("vfs: invalid filesystem id %d", vfs->filesystem_id);
    }

    kfree(vfs->mount_path);
    kfree(vfs);
    kfree(vfs_list);

    vfs_list = new_vfs_list;
    vfs_list_count--;

    return VFS_SUCCESS;
}

int vfs_mount(const char *path, struct gendisk *disk)
{
    struct vfs_info fs;
    size_t path_len = strlen(path) + 1;

    // find the filesystem that os used on the device
    if (fat32_load(&fs.fs_metadata.fat32, disk->driver_wrapper) == FAT32_SUCCESS)
    {
        fs.filesystem_id = VFS_FAT32;
        goto found_fs;
    }
    else if (ext2_load(&fs.fs_metadata.ext2, disk->driver_wrapper) == EXT2_SUCCESS)
    {
        fs.filesystem_id = VFS_EXT2;
        goto found_fs;
    }

    printk(KERN_WARNING "vfs: invalid filesystem on device %s", disk->block_device->path);
    return VFS_INVALID_FS;
found_fs:
    fs.mount_path = kmalloc(path_len);
    memcpy(fs.mount_path, path, path_len);
    vfs_append(fs);

    return VFS_SUCCESS;
}

int vfs_mount_root(struct gendisk *disk)
{
    return vfs_mount("/", disk);
}

struct vfs_directory *vfs_opendir(const char *path)
{
    struct vfs_directory *dir;
    struct vfs_info *vfs = vfs_get_structure(path);

    if (!vfs)
        return NULL;

    dir = kmalloc(sizeof(struct vfs_directory));
    dir->vfs = vfs;

    return dir;
}

void vfs_closedir(struct vfs_directory *directory)
{
    if (directory)
    {
        // ...
        kfree(directory);
    }
}

struct vfs_entry *vfs_readdir(struct vfs_directory *directory)
{
}
