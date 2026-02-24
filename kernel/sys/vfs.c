#include <kernel/inari.h>
#include <kernel/sys/block.h>
#include <kernel/sys/vfs.h>
#include <kernel/sys/stat.h>
#include <kernel/proc/proc.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/errno.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/sched.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/format.h>

static LIST_HEAD(vfs_layers);
static LIST_HEAD(vfs_mount_points);
static LIST_HEAD(vfs_handles);
static vfs_handle_t last_handle = 0xff0;

struct vfs_handle
{
    int32_t id;
    struct vfs_mount_point *mount;
    void *data;

    pid_t proc;

    struct list_head list;
};

int vfs_init()
{

    return 0;
}

int vfs_alloc_handle(struct vfs_mount_point *mount, vfs_handle_t *handle, void *data)
{
    tid_t tid;
    struct thread *th;
    struct vfs_handle *base;

    if (!handle) return -EINVAL;
    base = (struct vfs_handle*)kmalloc(sizeof(struct vfs_handle));
    if (!base)   return -ENOMEM;
    base->id = last_handle++;
    base->mount = mount;
    base->data = data;
    base->proc = 0;

    /* Link handle to a process */
    if (sched_current_thread(&tid) != 0)
        goto end;
    if (sched_get_thread(tid, &th) != 0)
        goto end;
    if (!tid || !th->proc_data)
        goto end;
    base->proc = th->proc_data->pid;
end:
    list_add(&base->list, &vfs_handles);
    *handle = base->id;
    return 0;
}

void vfs_kill_proc_handles(pid_t proc_pid)
{
    struct list_head *pos, *n;
    struct vfs_handle *entry;

    list_for_each_safe(pos, n, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->proc == proc_pid)
        {
            if (entry->mount->layer->ops->close)
                entry->mount->layer->ops->close(entry->mount, entry->id);
            list_del(&entry->list);
            kfree(entry);
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
    struct device *bdev = block_get(dev);

    struct vfs_mount_point *mount = (struct vfs_mount_point*)kmalloc(sizeof(struct vfs_mount_point));
    if (!mount) return -ENOMEM;
    mount->bdev = dev;
    mount->mount_point = path;
    INIT_LIST_HEAD(&mount->list);

    /* TODO: When mounting, ensure there's actual folder we can mount to */

    list_for_each(pos, &vfs_layers) {
        entry = list_entry(pos, struct vfs_layer, list);
        if (entry->mount(mount) == 0)
        {
            mount->layer = entry;
            list_add(&mount->list, &vfs_mount_points);
            if (bdev)
                printk("vfs: mounted %s; fs %s on dev:blk_%s%d",
                    mount->mount_point, mount->fs_name, bdev->group->name, DEVID(dev));
            else
                printk("vfs: mounted %s; fs %s",
                    mount->mount_point, mount->fs_name);
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
    struct device *bdev;

    list_for_each(pos, &vfs_mount_points) {
        entry = list_entry(pos, struct vfs_mount_point, list);
        if (strcmp(path, entry->mount_point) == 0)
        {
            bdev = block_get(entry->bdev);

            entry->layer->unmount(entry);
            if (bdev) printk("vfs: unmounted %s on dev:blk_%s%d",entry->fs_name, bdev->group->name, DEVID(bdev->dev));
            else printk("vfs: unmounted %s on dev:blk_invalid", entry->fs_name);
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
    struct device *bdev;

    /* Search mount point that owns this path */
    list_for_each(pos, &vfs_mount_points) {
        entry = list_entry(pos, struct vfs_mount_point, list);

        /* TODO: we use dumb approach here: just check that the mount point path
         *       IS in the beginning of the target path.
         *
         *       This allows to say something like "open /media/drive/boot.cfg", and this code WILL
         *       find required mount point. Things break when we specify something like `/media//drive/boot.cfg` */
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
                res = entry->mount->layer->ops->close(entry->mount, handle);
            list_del(&entry->list);
            kfree(entry);
            return res;
        }
    }

    return -EBADHNDL;
}

int vfs_read(vfs_handle_t handle, void *buf, size_t len, size_t *rlen)
{
    if (!buf) return -EINVAL;
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            if (entry->mount->layer->ops->read)
                return entry->mount->layer->ops->read(entry->mount, handle, buf, len, rlen);
            return -ENOSYS;
        }
    }

    return -EBADHNDL;
}

int vfs_write(vfs_handle_t handle, const void *buf, size_t sz)
{
    if (!buf) return -EINVAL;
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            if (entry->mount->layer->ops->write)
                return entry->mount->layer->ops->write(entry->mount, handle, buf, sz);
            return -ENOSYS;
        }
    }

    return -EBADHNDL;
}

int vfs_ioctl(vfs_handle_t handle, unsigned long req, void *arg)
{
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            if (entry->mount->layer->ops->ioctl)
                return entry->mount->layer->ops->ioctl(entry->mount, handle, req, arg);
            return -ENOSYS;
        }
    }

    return -EBADHNDL;
}

int vfs_seek(vfs_handle_t handle, size_t offset)
{
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            if (entry->mount->layer->ops->seek)
                return entry->mount->layer->ops->seek(entry->mount, handle, offset);
            return -ENOSYS;
        }
    }

    return -EBADHNDL;
}

int vfs_tell(vfs_handle_t handle, size_t *offset)
{
    if (!offset) return -EINVAL;
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            if (entry->mount->layer->ops->tell)
                return entry->mount->layer->ops->tell(entry->mount, handle, offset);
            return -ENOSYS;
        }
    }

    return -EBADHNDL;
}

int vfs_size(vfs_handle_t handle, size_t *size)
{
    if (!size) return -EINVAL;
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            if (entry->mount->layer->ops->size)
                return entry->mount->layer->ops->size(entry->mount, handle, size);
            return -ENOSYS;
        }
    }

    return -EBADHNDL;
}

int vfs_readdir(const char *path, struct vfs_node *node)
{
    if (!node || !path) return -EINVAL;

    struct list_head *pos;
    struct vfs_mount_point *entry;
    int res = 0, total_files = 0, found_dir = 0;

    /* Search mount point that owns this path */
    list_for_each(pos, &vfs_mount_points) {
        entry = list_entry(pos, struct vfs_mount_point, list);

        /* TODO: we use dumb approach here: just check that the mount point path
         *       IS in the beginning of the target path.
         *
         *       This allows to say something like "open /media/drive/boot.cfg", and this code WILL
         *       find required mount point. Things break when we specify something like `/media//drive/boot.cfg` */
        if (entry->layer->ops->readdir && strncmp(entry->mount_point, path, strlen(entry->mount_point)) == 0)
        {
            res = entry->layer->ops->readdir(entry, path, node, total_files);
            if (res != -ENOENT) found_dir = 1;
            if (res > 0)
                total_files += res;
            // if (total_files > node->off)
            break;
        }
    }

    if (!found_dir) return -ENOENT;

    node->off++;
    return res;
}
