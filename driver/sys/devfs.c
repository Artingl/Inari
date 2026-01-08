#ifdef CONFIG_DRV_DEVFS
#include <kernel/module.h>
#include <kernel/errno.h>
#include <kernel/sys/vfs.h>
#include <kernel/sys/block.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/printk.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/format.h>

/* Lists for each group of drivers */
static LIST_HEAD(sys_group);
static LIST_HEAD(disks_group);
static LIST_HEAD(volume_group);
static LIST_HEAD(input_group);
static LIST_HEAD(terminals_group);
static LIST_HEAD(video_group);
static LIST_HEAD(network_group);

static size_t devfs_groups_count = 7;
static const char *devfs_groups[] = { "sys", "disks", "volume", "input", "terminals", "video", "network" };
static struct list_head *devfs_groups_list[] = {
    &sys_group,
    &disks_group,
    &volume_group,
    &input_group,
    &terminals_group,
    &video_group,
    &network_group };

static uint8_t is_mounted;

struct devfs_group_item
{
    dev_t dev;
    struct list_head list;
};

static struct list_head *get_dev_group(dev_t dev, char **name)
{
    struct list_head *group = NULL;
    if (ISGROUP(DRVID(dev), DRIVER_SYS_GROUP))            { *name = "sys"; group = &sys_group; }
    else if (ISGROUP(DRVID(dev), DRIVER_DISKS_GROUP))     { *name = "disks"; group = &disks_group; }
    else if (ISGROUP(DRVID(dev), DRIVER_VOLUME_GROUP))    { *name = "volume"; group = &volume_group; }
    else if (ISGROUP(DRVID(dev), DRIVER_INPUT_GROUP))     { *name = "input"; group = &input_group; }
    else if (ISGROUP(DRVID(dev), DRIVER_TERMINALS_GROUP)) { *name = "terminals"; group = &terminals_group; }
    else if (ISGROUP(DRVID(dev), DRIVER_VIDEO_GROUP))     { *name = "video"; group = &video_group; }
    else if (ISGROUP(DRVID(dev), DRIVER_NETWORK_GROUP))   { *name = "network"; group = &network_group; }

    return group;
}

static void handle_block_dev_load(dev_t dev)
{
    struct list_head *group;
    struct block_device *bdev = block_get(dev);
    char *name;
    if (!(group = get_dev_group(dev, &name)) || !bdev)
        return; /* Ignore invalid driver groups */

    /* Put the bdev into appropriate driver group */
    struct devfs_group_item *item = (struct devfs_group_item*)kmalloc(sizeof(struct devfs_group_item));
    item->dev = dev;
    INIT_LIST_HEAD(&item->list);
    list_add(&item->list, group);
    struct list_head *pos;
    struct devfs_group_item *entry;

    printk("devfs: new entry dev:%s%d in group %s", bdev->group->name, DEVID(dev), name);
}

static void handle_block_dev_unload(dev_t dev)
{
    struct list_head *group;
    struct block_device *bdev = block_get(dev);
    char *name;
    if (!(group = get_dev_group(dev, &name)) || !bdev)
        return; /* Ignore invalid driver groups */

    struct list_head *pos;
    struct devfs_group_item *entry;

    list_for_each(pos, group) {
        entry = list_entry(pos, struct devfs_group_item, list);
        if (entry->dev == dev)
        {
            printk("devfs: removed entry dev:%s%d in group %s", bdev->group->name, DEVID(dev), name);
            list_del(&entry->list);
            kfree(entry);
            break;
        }
    }
}

static struct devfs_group_item *get_item_by_handle(vfs_handle_t handle)
{
    /* TODO: item might have been deleted while it was opened. Ensure we still have access to the item */
    return (struct devfs_group_item*)vfs_handle_data(handle);
}

static int find_group_by_path(struct list_head **group, char **group_path, const char *path)
{
    size_t i;
    if (strncmp(path, "/dev/", 5) == 0)
    {
        /* Determine in what group we are */
        for (i = 0; i < devfs_groups_count; i++)
        {
            /* &path[5] => skip /dev/ part in the path */
            if (strncmp(&path[5], devfs_groups[i], strlen(devfs_groups[i])) == 0)
            {
                if (group_path)
                    *group_path = devfs_groups[i];
                *group = devfs_groups_list[i];
                return 0;
            }
        }
    }
    
    return -ENOENT; /* File not found */
}

static int devfs_readdir(struct vfs_mount_point *mount, const char *path, struct vfs_node *node, size_t offset)
{
    size_t i;
    struct list_head *group = NULL;
    struct list_head *pos;
    struct devfs_group_item *entry;
    struct block_device *bdev;
    char dev_name[256];
    if (!mount || !path || !node) return -EINVAL;

    /* Reading from root */
    if (strcmp(path, "/") == 0)
    {
        strcpy(&node->name[0], "dev");
        node->st_mode = VFS_STAT_DIR;
        return 1;
    }
    /* Reading just /dev */
    else if (strcmp(path, "/dev") == 0)
    {
        for (i = 0; i < devfs_groups_count; i++)
        {
            if (i < node->off - offset) continue;
            strcpy(&node->name[0], devfs_groups[i]);
            node->st_mode = VFS_STAT_DIR;
            return 1;
        }
    }
    /* Reading /dev/GROUP */
    else if (strncmp(path, "/dev/", 5) == 0)
    {
        if (find_group_by_path(&group, NULL, path) != 0)
            return 0;

        /* Iterate through all nodes in the group */
        i = 0;
        list_for_each(pos, group) {
            entry = list_entry(pos, struct devfs_group_item, list);
            bdev = block_get(entry->dev);
            if (!bdev) continue;
            if (i++ < node->off - offset) continue;
            
            sprintf(&dev_name[0], "blk_%s%d", bdev->group->name, DEVID(bdev->dev));
            strcpy(&node->name[0], &dev_name[0]);
            node->st_mode = VFS_STAT_BLOCK | VFS_STAT_BLOCK;
            return 1;
        }
    }

    return 0;
}

static int devfs_read(struct vfs_mount_point *mount, vfs_handle_t handle, void *buf, size_t len, size_t *rlen)
{
    if (!mount || !buf) return -EINVAL;
    struct block_device *bdev;
    struct devfs_group_item *item = get_item_by_handle(handle);
    if (!item) return -ENOENT;
    bdev = block_get(item->dev);
    if (!bdev) return -ENODEV;
    *rlen = len;
    size_t block = len / bdev->group->block_size;
    return bdev->ops->read_blocks(bdev, 0, buf, block <= 0 ? 1 : block);
}

static int devfs_write(struct vfs_mount_point *mount, vfs_handle_t handle, const void *buf, size_t sz)
{
    if (!mount || !buf) return -EINVAL;
    struct block_device *bdev;
    struct devfs_group_item *item = get_item_by_handle(handle);
    if (!item) return -ENOENT;
    bdev = block_get(item->dev);
    if (!bdev) return -ENODEV;
    return bdev->ops->write_blocks(bdev, sz, buf, 0);
}

static int devfs_close(struct vfs_mount_point *mount, vfs_handle_t handle)
{
    if (!mount) return -EINVAL;
    struct devfs_group_item *item = get_item_by_handle(handle);
    if (item)
        vfs_kill_handle(handle);
    return 0;
}

static int devfs_open(struct vfs_mount_point *mount, vfs_handle_t *handle, const char *path, int flags)
{
    int res;
    size_t i;
    struct list_head *pos;
    struct devfs_group_item *entry;
    struct block_device *bdev;
    struct list_head *group = NULL;
    char *group_name;
    char dev_name[256];
    if (!mount) return -EINVAL;

    if (find_group_by_path(&group, &group_name, path) != 0)
        return -ENOENT;

    /* Iterate through all nodes in the group to find required file */
    i = 0;
    list_for_each(pos, group) {
        entry = list_entry(pos, struct devfs_group_item, list);
        bdev = block_get(entry->dev);
        if (!bdev) continue;
        
        sprintf(&dev_name[0], "/dev/%s/blk_%s%d", group_name, bdev->group->name, DEVID(bdev->dev));
        if (strcmp(dev_name, path) == 0)
        {
            return vfs_alloc_handle(mount, handle, entry);
        }
    }

    return -ENOENT;
}

static int devfs_unmount(struct vfs_mount_point *mount)
{
    size_t i;
    struct list_head *group;
    struct block_device *bdev;
    struct list_head *pos;
    struct devfs_group_item *entry;

    /* Deallocate all devs */
    for (i = 0; i < devfs_groups_count; i++)
    {
        group = devfs_groups_list[i];
        list_for_each(pos, group)
        {
            entry = list_entry(pos, struct devfs_group_item, list);
            bdev = block_get(entry->dev);
            printk("devfs: removed entry dev:%s%d in group %s", bdev->group->name, DEVID(entry->dev), devfs_groups[i]);
            list_del(&entry->list);
            kfree(entry);
        }
    }

    is_mounted = 0;
    return 0;
}

static int devfs_mount(struct vfs_mount_point *mount)
{
    if (!mount) return -EINVAL;
    if (strcmp(mount->mount_point, "/dev") == 0)
    {
        /* It is possible that some block devs were loaded before devfs was mounted. */
        dev_t devs[128];
        int offset = 0, count = 0;
        while ((count = block_get_refs(&devs[0], offset, 128)) > 0)
        {
            for (; count > 0; count--)
                handle_block_dev_load(devs[count - 1]);
            offset += 128;
        }

        is_mounted = 1;
        mount->fs_name = "devfs";
        return 0;
    }

    return -EINVFS;
}

static struct vfs_layer_ops devfs_ops = {
    .readdir = &devfs_readdir,
    .read = &devfs_read,
    .write = &devfs_write,
    .open = &devfs_open,
    .close = &devfs_close,
};

static struct vfs_layer devfs_layer = {
    .name = "devfs",
    .mount = &devfs_mount,
    .unmount = &devfs_unmount,
    .ops = &devfs_ops
};

int devfs_probe()
{
    is_mounted = 0;
    return vfs_add_layer(&devfs_layer);
}

int devfs_event_handler(event_t event)
{
    if (!is_mounted) return EVENT_HANDLED;
    switch (event.type)
    {
        case EVENT_LOAD_BLKDEV:
            handle_block_dev_load(event.as.blkdev);
            break;
        case EVENT_UNLOAD_BLKDEV:
            handle_block_dev_unload(event.as.blkdev);
            break;
    }

    return EVENT_HANDLED;
}

void devfs_cleanup()
{
    is_mounted = 0;
}

module_t devfs_module = {
    .probe = devfs_probe,
    .cleanup = devfs_cleanup,
    .event_bus = devfs_event_handler
};

module_register(
    "devfs_module",
    devfs_module
);

#endif