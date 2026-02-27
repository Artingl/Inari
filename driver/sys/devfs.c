#ifdef CONFIG_DRV_DEVFS
#include <kernel/errno.h>
#include <kernel/kprintf.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/module.h>
#include <kernel/sys/block.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/vfs.h>

#include <misc/format.h>
#include <misc/list.h>
#include <misc/string.h>

/* Lists for each group of drivers */
static LIST_HEAD(sys_group);
static LIST_HEAD(disks_group);
static LIST_HEAD(volume_group);
static LIST_HEAD(input_group);
static LIST_HEAD(terminals_group);
static LIST_HEAD(video_group);
static LIST_HEAD(network_group);

static size_t devfs_groups_count = 7;
static const char *devfs_groups[] = {"sys", "disks", "volume", "input", "terminals", "video", "network"};
static struct list_head *devfs_groups_list[] = {&sys_group,       &disks_group, &volume_group, &input_group,
                                                &terminals_group, &video_group, &network_group};

static uint8_t is_mounted;

struct devfs_group_item {
    dev_t dev;
    uint8_t is_blk;
    struct list_head list;
};

static struct list_head *get_dev_group(dev_t dev, char **name) {
    struct list_head *group = NULL;
    if (ISGROUP(DRVID(dev), DRIVER_SYS_GROUP)) {
        *name = "sys";
        group = &sys_group;
    } else if (ISGROUP(DRVID(dev), DRIVER_DISKS_GROUP)) {
        *name = "disks";
        group = &disks_group;
    } else if (ISGROUP(DRVID(dev), DRIVER_VOLUME_GROUP)) {
        *name = "volume";
        group = &volume_group;
    } else if (ISGROUP(DRVID(dev), DRIVER_INPUT_GROUP)) {
        *name = "input";
        group = &input_group;
    } else if (ISGROUP(DRVID(dev), DRIVER_TERMINALS_GROUP)) {
        *name = "terminals";
        group = &terminals_group;
    } else if (ISGROUP(DRVID(dev), DRIVER_VIDEO_GROUP)) {
        *name = "video";
        group = &video_group;
    } else if (ISGROUP(DRVID(dev), DRIVER_NETWORK_GROUP)) {
        *name = "network";
        group = &network_group;
    }

    return group;
}

static void handle_dev_load(uint8_t is_blk, dev_t dev) {
    struct list_head *group;
    struct device *bdev = NULL;
    struct device *chardev = NULL;
    char *name, *group_name = "invalid";
    if (is_blk && (bdev = block_get(dev)))
        group_name = bdev->group->name;
    else if ((chardev = char_get(dev)))
        group_name = chardev->group->name;
    else if (!bdev && !chardev)
        return;

    if (!(group = get_dev_group(dev, &name)))
        return; /* Ignore invalid driver groups */

    /* Put the bdev into appropriate driver group */
    struct devfs_group_item *item = (struct devfs_group_item *)kmalloc(sizeof(struct devfs_group_item));
    item->dev = dev;
    item->is_blk = is_blk;
    INIT_LIST_HEAD(&item->list);
    list_add(&item->list, group);

#ifdef CONFIG_DEBUG
    kprintf("devfs: new entry dev:%s_%s%d in group %s", (is_blk ? "blk" : "char"), group_name, DEVID(dev), name);
#endif
}

static void handle_dev_unload(dev_t dev) {
    struct list_head *group;
    struct device *bdev = NULL;
    struct device *chardev = NULL;
    char *name, *group_name = "invalid";
    if (!(group = get_dev_group(dev, &name)))
        return; /* Ignore invalid driver groups */

    struct list_head *pos;
    struct devfs_group_item *entry;

    list_for_each(pos, group) {
        entry = list_entry(pos, struct devfs_group_item, list);
        if (entry->dev == dev) {
            if (entry->is_blk && (bdev = block_get(dev)))
                group_name = bdev->group->name;
            else if ((chardev = char_get(dev)))
                group_name = chardev->group->name;

#ifdef CONFIG_DEBUG
            kprintf("devfs: removed entry dev:%s_%s%d in group %s", (entry->is_blk ? "blk" : "char"), group_name,
                    DEVID(dev), name);
#endif
            list_del(&entry->list);
            kfree(entry);
            break;
        }
    }
}

static struct devfs_group_item *get_item_by_handle(vfs_handle_t handle) {
    /* TODO: item might have been deleted while it was opened. Ensure we still have access to the item */
    return (struct devfs_group_item *)vfs_handle_data(handle);
}

static int find_group_by_path(struct list_head **group, char **group_path, const char *path) {
    size_t i;
    if (strncmp(path, "/devices/", 9) == 0) {
        /* Determine in what group we are */
        for (i = 0; i < devfs_groups_count; i++) {
            /* &path[9] => skip /devices/ part in the path */
            if (strncmp(&path[9], devfs_groups[i], strlen(devfs_groups[i])) == 0) {
                if (group_path)
                    *group_path = (char *)devfs_groups[i];
                *group = devfs_groups_list[i];
                return 0;
            }
        }
    }

    return -ENOENT; /* File not found */
}

static int devfs_readdir(struct vfs_mount_point *mount, const char *path, struct vfs_node *node, size_t offset) {
    size_t i;
    struct list_head *group = NULL;
    struct list_head *pos;
    struct devfs_group_item *entry;
    struct device *bdev = NULL;
    struct device *chardev = NULL;
    char *group_name = NULL;
    char dev_name[256];
    if (!mount || !path || !node)
        return -EINVAL;

    /* Reading from root */
    if (strcmp(path, "/") == 0) {
        strcpy(&node->name[0], "devices");
        node->st_mode = VFS_STAT_DIR;
        return 1;
    }
    /* Reading just /devices */
    else if (strcmp(path, "/devices") == 0) {
        for (i = 0; i < devfs_groups_count; i++) {
            if (i < node->off - offset)
                continue;
            strcpy(&node->name[0], devfs_groups[i]);
            node->st_mode = VFS_STAT_DIR;
            return 1;
        }
    }
    /* Reading /devices/GROUP */
    else if (strncmp(path, "/devices/", 9) == 0) {
        if (find_group_by_path(&group, NULL, path) != 0)
            return -ENOENT;

        /* Iterate through all nodes in the group */
        i = 0;
        list_for_each(pos, group) {
            entry = list_entry(pos, struct devfs_group_item, list);
            if (entry->is_blk && (bdev = block_get(entry->dev)))
                group_name = bdev->group->name;
            else if ((chardev = char_get(entry->dev)))
                group_name = chardev->group->name;
            if (!group_name)
                continue;
            if (i++ < node->off - offset)
                continue;

            sprintf(&dev_name[0], "%s_%s%d", (entry->is_blk ? "blk" : "char"), group_name, DEVID(entry->dev));
            strcpy(&node->name[0], &dev_name[0]);
            node->st_mode = VFS_STAT_FILE | (entry->is_blk ? VFS_STAT_BLOCK : VFS_STAT_CHAR);
            return 1;
        }

        return 0;
    }

    return -ENOENT;
}

static int devfs_read(struct vfs_mount_point *mount, vfs_handle_t handle, void *buf, size_t len, size_t *rlen) {
    if (!mount || !buf)
        return -EINVAL;
    struct device *chardev = NULL;
    struct device *bdev = NULL;
    struct devfs_group_item *item = get_item_by_handle(handle);
    if (!item)
        return -ENOENT;
    if (item->is_blk)
        bdev = block_get(item->dev);
    else
        chardev = char_get(item->dev);

    if (rlen)
        *rlen = len;
    if (bdev) {
        if (!((struct block_ops *)bdev->ops)->read_blocks)
            return -ENOSYS;
        size_t block = len / bdev->group->block_size;
        return ((struct block_ops *)bdev->ops)->read_blocks(bdev, 0, buf, block <= 0 ? 1 : block);
    } else {
        if (!((struct char_ops *)chardev->ops)->read)
            return -ENOSYS;
        return ((struct char_ops *)chardev->ops)->read(chardev, (uint8_t *)buf, len);
    }

    return -ENODEV;
}

static int devfs_write(struct vfs_mount_point *mount, vfs_handle_t handle, const void *buf, size_t sz) {
    if (!mount || !buf)
        return -EINVAL;
    struct device *chardev = NULL;
    struct device *bdev = NULL;
    struct devfs_group_item *item = get_item_by_handle(handle);
    if (!item)
        return -ENOENT;
    if (item->is_blk)
        bdev = block_get(item->dev);
    else
        chardev = char_get(item->dev);

    if (bdev) {
        if (!((struct block_ops *)bdev->ops)->write_blocks)
            return -ENOSYS;
        return ((struct block_ops *)bdev->ops)->write_blocks(bdev, sz, buf, 0);
    } else {
        if (!((struct char_ops *)chardev->ops)->write)
            return -ENOSYS;
        return ((struct char_ops *)chardev->ops)->write(chardev, (const uint8_t *)buf, sz);
    }

    return -ENODEV;
}

static int devfs_close(struct vfs_mount_point *mount, vfs_handle_t handle) {
    if (!mount)
        return -EINVAL;
    // struct devfs_group_item *item =
    get_item_by_handle(handle);
    return 0;
}

static int devfs_open(struct vfs_mount_point *mount, vfs_handle_t *handle, const char *path, int flags) {
    struct list_head *pos;
    struct devfs_group_item *entry;
    struct device *bdev = NULL;
    struct device *chardev = NULL;
    struct list_head *group = NULL;
    char *group_name;
    char *dev_group_name = NULL;
    char dev_name[256];
    if (!mount)
        return -EINVAL;

    if (find_group_by_path(&group, &group_name, path) != 0)
        return -ENOENT;

    /* Iterate through all nodes in the group to find required file */
    list_for_each(pos, group) {
        entry = list_entry(pos, struct devfs_group_item, list);
        if (entry->is_blk && (bdev = block_get(entry->dev)))
            dev_group_name = bdev->group->name;
        else if ((chardev = char_get(entry->dev)))
            dev_group_name = chardev->group->name;
        if (!dev_group_name)
            continue;

        sprintf(&dev_name[0], "/devices/%s/%s_%s%d", group_name, (entry->is_blk ? "blk" : "char"), dev_group_name,
                DEVID(entry->dev));
        if (strcmp(dev_name, path) == 0) {
            return vfs_alloc_handle(mount, handle, entry);
        }
    }

    return -ENOENT;
}

static int devfs_unmount(struct vfs_mount_point *mount) {
    // devfs_cleanup();
    return 0;
}

static int devfs_mount(struct vfs_mount_point *mount) {
    if (!mount)
        return -EINVAL;
    if (strcmp(mount->mount_point, "/devices") == 0) {
        /* It is possible that some block devs were loaded before devfs was mounted. */
        dev_t devs[128];
        int offset = 0, count = 0;
        while ((count = block_get_refs(&devs[0], offset, 128)) > 0) {
            for (; count > 0; count--)
                handle_dev_load(1, devs[count - 1]);
            offset += 128;
        }

        /* Same for char devices */
        offset = 0, count = 0;
        while ((count = char_get_refs(&devs[0], offset, 128)) > 0) {
            for (; count > 0; count--)
                handle_dev_load(0, devs[count - 1]);
            offset += 128;
        }

        is_mounted = 1;
        mount->fs_name = "devfs";
        return 0;
    }

    return -EINVFS;
}

static int devfs_ioctl(struct vfs_mount_point *mount, vfs_handle_t handle, unsigned long req, void *arg) {
    if (!mount)
        return -EINVAL;
    struct device *chardev = NULL;
    struct device *bdev = NULL;
    struct devfs_group_item *item = get_item_by_handle(handle);
    if (!item)
        return -ENOENT;
    if (item->is_blk)
        bdev = block_get(item->dev);
    else
        chardev = char_get(item->dev);

    if (bdev) {
        if (!((struct block_ops *)bdev->ops)->ioctl)
            return -ENOSYS;
        return ((struct block_ops *)bdev->ops)->ioctl(bdev, req, arg);
    } else {
        if (!((struct char_ops *)chardev->ops)->ioctl)
            return -ENOSYS;
        return ((struct char_ops *)chardev->ops)->ioctl(chardev, req, arg);
    }

    return -ENODEV;
}

static int devfs_flush(struct vfs_mount_point *mount, vfs_handle_t handle) {
    if (!mount)
        return -EINVAL;
    struct device *chardev = NULL;
    struct devfs_group_item *item = get_item_by_handle(handle);
    if (!item)
        return -ENOENT;
    if (item->is_blk)
        return -ENOSYS;
    else
        chardev = char_get(item->dev);

    if (chardev) {
        if (!((struct char_ops *)chardev->ops)->flush)
            return -ENOSYS;
        return ((struct char_ops *)chardev->ops)->flush(chardev);
    }

    return -ENODEV;
}

static struct vfs_layer_ops devfs_ops = {
    .readdir = &devfs_readdir,
    .read = &devfs_read,
    .write = &devfs_write,
    .open = &devfs_open,
    .close = &devfs_close,
    .ioctl = &devfs_ioctl,
    .flush = &devfs_flush,
};

static struct vfs_layer devfs_layer = {
    .name = "devfs", .mount = &devfs_mount, .unmount = &devfs_unmount, .ops = &devfs_ops};

static int devfs_probe() {
    is_mounted = 0;
    return vfs_add_layer(&devfs_layer);
}

static int devfs_event_handler(event_t event) {
    uint8_t is_blk = 1;
    if (!is_mounted)
        return EVENT_HANDLED;
    switch (event.type) {
    case EVENT_LOAD_CHARDEV:
        is_blk = 0; // fallthrough
    case EVENT_LOAD_BLKDEV:
        handle_dev_load(is_blk, event.as.dev);
        break;
    case EVENT_UNLOAD_CHARDEV:
    case EVENT_UNLOAD_BLKDEV:
        handle_dev_unload(event.as.dev);
        break;
    }

    return EVENT_HANDLED;
}

static void devfs_cleanup() {
    size_t i;
    struct list_head *group;
    struct device *bdev;
    struct list_head *pos, *n;
    struct devfs_group_item *entry;

    /* Deallocate all devs */
    for (i = 0; i < devfs_groups_count; i++) {
        group = devfs_groups_list[i];
        list_for_each_safe(pos, n, group) {
            entry = list_entry(pos, struct devfs_group_item, list);
            if (!entry)
                continue;

            bdev = block_get(entry->dev);
#ifdef CONFIG_DEBUG
            if (bdev)
                kprintf("devfs: removed entry dev:%s%d in group %s", bdev->group->name, DEVID(entry->dev),
                        devfs_groups[i]);
#endif
            list_del(pos);
            kfree(entry);
        }
    }

    vfs_remove_layer(&devfs_layer);
    is_mounted = 0;
}

module_t devfs_module = {.probe = devfs_probe, .cleanup = devfs_cleanup, .event_bus = devfs_event_handler};

module_register("devfs", devfs_module);

#endif