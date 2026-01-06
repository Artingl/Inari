#ifdef CONFIG_DRV_DEVFS
#include <kernel/module.h>
#include <kernel/errno.h>
#include <kernel/sys/vfs.h>

#include <misc/string.h>

static int devfs_mount_bdev(struct vfs_mount_point *mount)
{
    if (!mount) return -EINVAL;
    if (strcmp(mount->mount_point, "/dev") == 0)
    {
        mount->fs_name = "devfs";
        return 0;
    }

    return -EINVFS;
}

static int devfs_readdir(struct vfs_mount_point *mount, const char *path, struct vfs_node *nodes, size_t offset, size_t limit)
{
    if (!mount || !path || !nodes) return -EINVAL;
    size_t i = 0;

    if (strcmp(path, "/") == 0)
    {
        strcpy(&nodes[i].name[0], "dev");
        nodes[i++].st_mode = VFS_STAT_DIR;
    }
    else if (strncmp(path, "/dev", 4) == 0)
    {
        strcpy(&nodes[i].name[0], "sys");
        nodes[i++].st_mode = VFS_STAT_DIR;
        strcpy(&nodes[i].name[0], "disks");
        nodes[i++].st_mode = VFS_STAT_DIR;
        strcpy(&nodes[i].name[0], "volume");
        nodes[i++].st_mode = VFS_STAT_DIR;
        strcpy(&nodes[i].name[0], "input");
        nodes[i++].st_mode = VFS_STAT_DIR;
        strcpy(&nodes[i].name[0], "terminals");
        nodes[i++].st_mode = VFS_STAT_DIR;
        strcpy(&nodes[i].name[0], "video");
        nodes[i++].st_mode = VFS_STAT_DIR;
        strcpy(&nodes[i].name[0], "network");
        nodes[i++].st_mode = VFS_STAT_DIR;
    }

    return i;
}

static struct vfs_layer_ops devfs_ops = {
    .readdir = &devfs_readdir
};

static struct vfs_layer devfs_layer = {
    .name = "devfs",
    .mount = &devfs_mount_bdev,
    .ops = &devfs_ops
};

int devfs_probe()
{
    return vfs_add_layer(&devfs_layer);
}

void devfs_cleanup()
{
}

module_t devfs_module = {
    .probe = devfs_probe,
    .cleanup = devfs_cleanup
};

module_register(
    "devfs_module",
    devfs_module
);

#endif