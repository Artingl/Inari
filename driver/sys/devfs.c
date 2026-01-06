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

static size_t devfs_groups_count = 7;
static const char *devfs_groups[] = { "sys", "disks", "volume", "input", "terminals", "video", "network" };

static int devfs_readdir(struct vfs_mount_point *mount, const char *path, struct vfs_node *node, size_t offset)
{
    size_t i;
    if (!mount || !path || !node) return -EINVAL;

    if (strcmp(path, "/") == 0)
    {
        strcpy(&node->name[0], "dev");
        node->st_mode = VFS_STAT_DIR;
        return 1;
    }
    else if (strncmp(path, "/dev", 4) == 0)
    {
        for (i = 0; i < devfs_groups_count; i++)
        {
            if (i < node->off - offset) continue;
            strcpy(&node->name[0], devfs_groups[i]);
            node->st_mode = VFS_STAT_DIR;
            return 1;
        }
    }

    return 0;
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