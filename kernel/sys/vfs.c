#include <kernel/inari.h>
#include <kernel/sys/block.h>
#include <kernel/sys/vfs.h>
#include <kernel/sys/stat.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/errno.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/format.h>

LIST_HEAD(vfs_layers);
LIST_HEAD(vfs_mount_points);
LIST_HEAD(vfs_handles);

vfs_handle_t last_handle = 0xff0;
struct list_head handles;

struct vfs_handle
{
    int32_t id;
    struct vfs_mount_point *mount;
    void *data;

    struct list_head list;
};

int vfs_init()
{

    return 0;
}

int vfs_alloc_handle(struct vfs_mount_point *mount, vfs_handle_t *handle, void *data)
{
    if (!handle) return -EINVAL;
    struct vfs_handle *base = (struct vfs_handle*)kmalloc(sizeof(struct vfs_handle));
    base->id = last_handle++;
    base->mount = mount;
    base->data = data;
    list_add(&base->list, &vfs_handles);
    *handle = base->id;
    return 0;
}

void vfs_kill_handle(vfs_handle_t handle)
{
    struct list_head *pos;
    struct vfs_handle *entry;
   
    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            kfree(entry);
            return;
        }
    }
}

void *vfs_handle_data(vfs_handle_t handle)
{
    struct list_head *pos;
    struct vfs_handle *entry;
   
    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
            return entry->data;
    }

    return NULL;
}

int vfs_mount(dev_t dev, const char* path)
{
    struct list_head *pos;
    struct vfs_layer *entry;
    struct block_device *bdev = block_get(dev);

    if (!bdev) return -ENODEV;

    struct vfs_mount_point *mount = (struct vfs_mount_point*)kmalloc(sizeof(struct vfs_mount_point));
    mount->bdev = dev;
    mount->mount_point = path;
    INIT_LIST_HEAD(&mount->list);

    list_for_each(pos, &vfs_layers) {
        entry = list_entry(pos, struct vfs_layer, list);
        if (entry->mount(mount) == 0)
        {
            mount->layer = entry;
            list_add(&mount->list, &vfs_mount_points);
            printk("vfs: mounted %s; fs %s on dev:%s%d",
                mount->mount_point, mount->fs_name, bdev->group->name, DEVID(dev));
            return 0;
        }
    }

    kfree(mount);
    return -EINVFS;
}

int vfs_unmount(const char* path)
{
    struct list_head *pos;
    struct vfs_mount_point *entry;
    struct block_device *bdev;

    list_for_each(pos, &vfs_mount_points) {
        entry = list_entry(pos, struct vfs_mount_point, list);
        if (strcmp(path, entry->mount_point) == 0)
        {
            bdev = block_get(entry->bdev);

            entry->layer->unmount(entry);
            if (bdev) printk("vfs: unmounted %s on dev:%s%d",entry->fs_name, bdev->group->name, DEVID(bdev->dev));
            else printk("vfs: unmounted %s on dev:invalid", entry->fs_name);
            kfree(entry);
            list_del(&entry->list);
            return 0;
        }
    }

    return -EINVAL;
}

int vfs_add_layer(struct vfs_layer *layer)
{
    if (!layer) return -EINVAL;
    INIT_LIST_HEAD(&layer->list);
    list_add(&layer->list, &vfs_layers);
    printk("vfs: added new layer %s", layer->name);
    return 0;
}

int vfs_remove_layer(struct vfs_layer *layer)
{
    if (!layer) return -EINVAL;
    list_del(&layer->list);
    /* TODO: Unmount nodes that use this layer */
    printk("vfs: removed layer %s", layer->name);
    return 0;
}

int vfs_open(vfs_handle_t *file, const char *path, int flags)
{
    if (!file) return -EINVAL;

    struct list_head *pos;
    struct vfs_mount_point *entry;
    struct block_device *bdev;

    /* Search mount point that owns this path */
    list_for_each(pos, &vfs_mount_points) {
        entry = list_entry(pos, struct vfs_mount_point, list);

        /* TODO: we use dumb approach here: just check that the mount point path
         *       IS in the beginning of the target path.
         *
         *       This allows to say something like "open /media/drive/boot.cfg", and this code WILL
         *       find required mount point. Things break when we specify something like `/media//drive/boot.cfg */
        if (entry->layer->ops->open && strncmp(entry->mount_point, path, strlen(entry->mount_point)) == 0)
        {
            return entry->layer->ops->open(entry, file, path, flags);
        }
    }

    return -ENOENT;
}

int vfs_close(vfs_handle_t handle)
{
    struct list_head *pos;
    struct vfs_handle *entry;
    int res = 0;
   
    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            if (entry->mount->layer->ops->close)
                res = entry->mount->layer->ops->close(entry, handle);
            kfree(entry);
            return res;
        }
    }

    return -EBADHNDL;
}

int vfs_read(vfs_handle_t handle, void *buf, size_t len, size_t *rlen)
{
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            if (entry->mount->layer->ops->read)
                return entry->mount->layer->ops->read(entry, handle, buf, len, rlen);
            return -EINVAL;
        }
    }

    return -EBADHNDL;
}