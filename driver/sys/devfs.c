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

struct devfs_group_item
{
    dev_t dev;
    struct list_head list;
};


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

static size_t devfs_groups_count = 7;
static const char *devfs_groups[] = { "sys", "disks", "volume", "input", "terminals", "video", "network" };

static int devfs_readdir(struct vfs_mount_point *mount, const char *path, struct vfs_node *node, size_t offset)
{
    size_t i;
    struct list_head *group = NULL;
    struct list_head *pos;
    struct devfs_group_item *entry;
    struct block_device *bdev;
    struct list_head *devfs_groups_list[] = {
        &sys_group,
        &disks_group,
        &volume_group,
        &input_group,
        &terminals_group,
        &video_group,
        &network_group };
    char bdev_name[256];
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
        /* Determine in what group we are */
        for (i = 0; i < devfs_groups_count; i++)
        {
            /* &path[5] => skip /dev/ part in the path */
            if (strcmp(&path[5], devfs_groups[i]) == 0)
            {
                group = devfs_groups_list[i];
                break;
            }
        }

        if (!group) return 0;

        /* Iterate through all nodes in the group */
        i = 0;
        list_for_each(pos, group) {
            entry = list_entry(pos, struct devfs_group_item, list);
            bdev = block_get(entry->dev);
            if (!bdev) continue;
            if (i++ < node->off - offset) continue;
            
            sprintf(&bdev_name[0], "%s%d", bdev->group->name, DEVID(bdev->dev));
            strcpy(&node->name[0], &bdev_name[0]);
            node->st_mode = VFS_STAT_BLOCK;
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
    /* During initial devfs module load, we analyze all loaded block devs.
     * It is possible that some block devs were loaded before devfs module
     * itself was loaded. */
    dev_t devs[128];
    int offset = 0, count = 0;
    while ((count = block_get_refs(&devs[0], offset, 128)) > 0)
    {
        for (; count > 0; count--)
            handle_block_dev_load(devs[count - 1]);
        offset += 128;
    }

    return vfs_add_layer(&devfs_layer);
}

int devfs_event_handler(event_t event)
{
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