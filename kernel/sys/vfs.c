#include <kernel/inari.h>
#include <kernel/sys/block.h>
#include <kernel/sys/vfs.h>
#include <kernel/sys/stat.h>
#include <kernel/proc/proc.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/errno.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/sched.h>
#include <kernel/sync/spinlock.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/format.h>

static spinlock_t vfs_lock = {0};

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

static void normalize_path(const char *path, char *result)
{
    size_t i, off = 0;
    // uint8_t had_slash = 0;

    /* Remove duplicate slash, e.g. `//` */
    for (i = 0; i < VFS_PATH_SIZE && path[i]; i++)
    {
        // if (path[i] == '/' && had_slash)
        // { off++; continue; }
        // else if (path[i] == '/') had_slash = 1;
        // else had_slash = 0;

        result[i - off] = path[i];
    }
    result[i - off] = 0;
    // kprintf("-- %s %s", result, path);
}

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
            list_del(pos);
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
    char path_buf[VFS_PATH_SIZE];
    if (!path)  return -EINVAL;
    normalize_path(path, path_buf);
    struct list_head *pos;
    struct vfs_layer *entry;
    struct device *bdev = block_get(dev);

    struct vfs_mount_point *mount = (struct vfs_mount_point*)kmalloc(sizeof(struct vfs_mount_point));
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    if (!mount) return -ENOMEM;
    mount->bdev = dev;
    memcpy(mount->mount_point, path_buf, VFS_PATH_SIZE);
    INIT_LIST_HEAD(&mount->list);

    /* TODO: When mounting, ensure there's actual folder we can mount to */

    list_for_each(pos, &vfs_layers) {
        entry = list_entry(pos, struct vfs_layer, list);
        if (entry->mount(mount) == 0)
        {
            mount->layer = entry;
            list_add(&mount->list, &vfs_mount_points);
            if (bdev)
                kprintf("vfs: mounted %s; fs %s on dev:blk_%s%d",
                    mount->mount_point, mount->fs_name, bdev->group->name, DEVID(dev));
            else
                kprintf("vfs: mounted %s; fs %s",
                    mount->mount_point, mount->fs_name);
            
            spin_unlock_irqrestore(&vfs_lock, flags);
            return 0;
        }
    }

    spin_unlock_irqrestore(&vfs_lock, flags);
    kfree(mount);
    return -EINVFS;
}

int vfs_unmount(const char* path)
{
    char path_buf[VFS_PATH_SIZE];
    if (!path)  return -EINVAL;
    normalize_path(path, path_buf);
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    struct list_head *pos;
    struct vfs_mount_point *entry;
    struct device *bdev;

    list_for_each(pos, &vfs_mount_points) {
        entry = list_entry(pos, struct vfs_mount_point, list);
        if (strcmp(path_buf, entry->mount_point) == 0)
        {
            bdev = block_get(entry->bdev);

            entry->layer->unmount(entry);
            if (bdev) kprintf("vfs: unmounted %s on dev:blk_%s%d",entry->fs_name, bdev->group->name, DEVID(bdev->dev));
            else kprintf("vfs: unmounted %s on dev:blk_invalid", entry->fs_name);
            list_del(pos);
            kfree(entry);
            
            spin_unlock_irqrestore(&vfs_lock, flags);
            return 0;
        }
    }
    
    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EINVAL;
}

int vfs_add_layer(struct vfs_layer *layer)
{
    if (!layer) return -EINVAL;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    INIT_LIST_HEAD(&layer->list);
    list_add(&layer->list, &vfs_layers);
    kprintf("vfs: added new layer %s", layer->name);
    
    spin_unlock_irqrestore(&vfs_lock, flags);
    return 0;
}

int vfs_remove_layer(struct vfs_layer *layer)
{
    if (!layer) return -EINVAL;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    list_del(&layer->list);
    /* TODO: Unmount nodes that use this layer */
    kprintf("vfs: removed layer %s (TODO)", layer->name);
    
    spin_unlock_irqrestore(&vfs_lock, flags);
    return 0;
}

int vfs_open(vfs_handle_t *file, const char *path, int flags)
{
    char path_buf[VFS_PATH_SIZE];
    if (!path)  return -EINVAL;
    normalize_path(path, path_buf);
    if (!file) return -EINVAL;
    int res;
    uint32_t lock_flags;
    spin_lock_irqsave(&vfs_lock, lock_flags);

    struct list_head *pos;
    struct vfs_mount_point *entry;

    /* Search mount point that owns this path */
    list_for_each(pos, &vfs_mount_points) {
        entry = list_entry(pos, struct vfs_mount_point, list);

        if (strncmp(entry->mount_point, path_buf, strlen(entry->mount_point)) == 0)
        {
            res = -ENOSYS;
            spin_unlock_irqrestore(&vfs_lock, lock_flags);
            if (entry->layer->ops->open)
                res = entry->layer->ops->open(entry, file, path_buf, flags);
            return res;
        }
    }
    
    spin_unlock_irqrestore(&vfs_lock, lock_flags);
    return -ENOENT;
}

int vfs_close(vfs_handle_t handle)
{
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    struct list_head *pos;
    struct vfs_handle *entry;
    int res = 0;
   
    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            spin_unlock_irqrestore(&vfs_lock, flags);
            if (entry->mount->layer->ops->close)
                res = entry->mount->layer->ops->close(entry->mount, handle);
            list_del(pos);
            kfree(entry);
            
            return res;
        }
    }
    
    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EBADHNDL;
}

int vfs_flush(vfs_handle_t handle)
{
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    struct list_head *pos;
    struct vfs_handle *entry;
    int res = 0;
   
    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            spin_unlock_irqrestore(&vfs_lock, flags);
            if (entry->mount->layer->ops->flush)
                res = entry->mount->layer->ops->flush(entry->mount, handle);
            return res;
        }
    }
    
    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EBADHNDL;
}

int vfs_read(vfs_handle_t handle, void *buf, size_t len, size_t *rlen)
{
    if (!buf) return -EINVAL;
    int res;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            res = -ENOSYS;
            spin_unlock_irqrestore(&vfs_lock, flags);
            if (entry->mount->layer->ops->read)
                res = entry->mount->layer->ops->read(entry->mount, handle, buf, len, rlen);
            return res;
        }
    }
    
    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EBADHNDL;
}

int vfs_write(vfs_handle_t handle, const void *buf, size_t sz)
{
    if (!buf) return -EINVAL;
    int res;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            res = -ENOSYS;
            spin_unlock_irqrestore(&vfs_lock, flags);
            if (entry->mount->layer->ops->write)
                res = entry->mount->layer->ops->write(entry->mount, handle, buf, sz);
            return res;
        }
    }
    
    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EBADHNDL;
}

int vfs_ioctl(vfs_handle_t handle, unsigned long req, void *arg)
{
    int res;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);

    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            res = -ENOSYS;
            spin_unlock_irqrestore(&vfs_lock, flags);
            if (entry->mount->layer->ops->ioctl)
                res = entry->mount->layer->ops->ioctl(entry->mount, handle, req, arg);
            return res;
        }
    }

    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EBADHNDL;
}

int vfs_seek(vfs_handle_t handle, size_t offset)
{
    int res;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            res = -ENOSYS;
            spin_unlock_irqrestore(&vfs_lock, flags);
            if (entry->mount->layer->ops->seek)
                res = entry->mount->layer->ops->seek(entry->mount, handle, offset);
            return res;
        }
    }

    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EBADHNDL;
}

int vfs_tell(vfs_handle_t handle, size_t *offset)
{
    if (!offset) return -EINVAL;
    int res;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            res = -ENOSYS;
            spin_unlock_irqrestore(&vfs_lock, flags);
            if (entry->mount->layer->ops->tell)
                res = entry->mount->layer->ops->tell(entry->mount, handle, offset);
            return res;
        }
    }

    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EBADHNDL;
}

int vfs_size(vfs_handle_t handle, size_t *size)
{
    if (!size) return -EINVAL;
    int res;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);
    
    struct list_head *pos;
    struct vfs_handle *entry;

    list_for_each(pos, &vfs_handles) {
        entry = list_entry(pos, struct vfs_handle, list);
        if (entry->id == handle)
        {
            res = -ENOSYS;
            spin_unlock_irqrestore(&vfs_lock, flags);
            if (entry->mount->layer->ops->size)
                res = entry->mount->layer->ops->size(entry->mount, handle, size);
            return res;
        }
    }

    spin_unlock_irqrestore(&vfs_lock, flags);
    return -EBADHNDL;
}

int vfs_readdir(const char *path, struct vfs_node *node)
{
    char path_buf[VFS_PATH_SIZE];
    if (!path)  return -EINVAL;
    normalize_path(path, path_buf);
    if (!node) return -EINVAL;
    uint32_t flags;
    spin_lock_irqsave(&vfs_lock, flags);

    struct list_head *pos;
    struct vfs_mount_point *entry;
    int res = 0, total_files = 0, found_dir = 0;

    /* Search mount point that owns this path */
    list_for_each(pos, &vfs_mount_points) {
        entry = list_entry(pos, struct vfs_mount_point, list);

        if (entry->layer->ops->readdir && strncmp(entry->mount_point, path_buf, strlen(entry->mount_point)) == 0)
        {
            res = entry->layer->ops->readdir(entry, path_buf, node, total_files);
            if (res != -ENOENT) found_dir = 1;
            if (res > 0)
                total_files += res;
            // if (total_files > node->off)
            break;
        }
    }

    spin_unlock_irqrestore(&vfs_lock, flags);
    if (!found_dir) return -ENOENT;

    node->off++;
    return res;
}
