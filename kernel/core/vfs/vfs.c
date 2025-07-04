#include <kernel/kernel.h>
#include <kernel/driver/memory/memory.h>
#include <kernel/core/vfs/dev.h>
#include <kernel/core/vfs/vfs.h>
#include <kernel/core/lock/spinlock.h>
#include <kernel/list/dynlist.h>
#include <kernel/libc/string.h>
#include <kernel/core/errno.h>

struct mount_point
{
    const char *mount_point;
    struct dev_ref *device_pointer;
    device_handle handle;
    size_t opened_refs;
};

struct assigned_handle
{
    struct mount_point *mount_point;
    device_handle handle;
};

int64_t last_handle = 0xffff;
spinlock_t vfs_lock = {0};

dynlist_t mounts = {0};
dynlist_t assigned_handles = {0};

static int64_t vfs_get_mount(const char *mount_point)
{
    int64_t i;
    struct mount_point *point;

    // Search through the dynlist, to find the device at mount point
    for (i = 0; i < dynlist_size(mounts); i++)
    {
        point = dynlist_get(mounts, i, struct mount_point*);
        if (point != NULL && strcmp(point->mount_point, mount_point) == 0)
            return i;
    }

    return -1;
}

static int64_t vfs_get_mount_handle(device_handle handle)
{
    int64_t i;
    struct mount_point *point;

    // Search through the dynlist, to find the device with such handle
    for (i = 0; i < dynlist_size(mounts); i++)
    {
        point = dynlist_get(mounts, i, struct mount_point*);
        if (point != NULL && point->handle == handle)
            return i;
    }

    return -1;
}

static int64_t vfs_can_unmount(struct mount_point *point)
{
    // Check that the file is not opened by anything
    if (point->opened_refs > 0)
        return K_DRBUSY;
    
    return K_OKAY;
}

static int vfs_assign_handle(struct mount_point *point, device_handle handle)
{
    int64_t i;
    struct assigned_handle *assign;

    // Check that the handle is not in use
    for (i = 0; i < dynlist_size(assigned_handles); i++)
    {
        assign = dynlist_get(assigned_handles, i, struct assigned_handle*);
        if (assign != NULL && assign->handle == handle)
            // Handle is already in use
            return K_DRBUSY;
    }

    // Allocate the handle to this mount point
    assign = kmalloc(sizeof(struct assigned_handle));
    assign->mount_point = point;
    assign->handle = handle;

    // Add the device to the list
    dynlist_append(assigned_handles, point);

    return K_OKAY;
}

static int64_t vfs_get_assigned_point(device_handle handle)
{
    int64_t i;
    struct assigned_handle *assign;

    // Search through the dynlist, to find the device with this assigned handle
    for (i = 0; i < dynlist_size(assigned_handles); i++)
    {
        assign = dynlist_get(assigned_handles, i, struct assigned_handle*);
        if (assign != NULL && assign->handle == handle)
            return i;
    }

    return -1;
}

int vfs_init()
{
    printk("vfs: initializing");
    return K_OKAY;
}

device_handle vfs_mount(const char *mount_point, struct dev_ref *device)
{
    device_handle handle = last_handle++;
    spinlock_acquire(&vfs_lock);

    // Check that no devices are mounted at that mount point
    if (vfs_get_mount(mount_point) != -1)
        goto busy;
    
    // Create the device and mount it
    struct mount_point *point = kmalloc(sizeof(struct mount_point));
    point->mount_point = mount_point;
    point->handle = handle;
    point->device_pointer = device;

    // Add the device to the list
    dynlist_append(mounts, point);

    goto end;
busy:
    handle = K_DRBUSY;
end:
    spinlock_release(&vfs_lock);
    return handle;
}

int vfs_unmount_handle(device_handle handle)
{
    int code = K_OKAY;
    int64_t device_id;
    struct mount_point *point;
    spinlock_acquire(&vfs_lock);

    // Check that we have this device mounted
    if ((device_id = vfs_get_mount_handle(handle)) == -1)
        goto invalid_dev;
    point = dynlist_get(mounts, device_id, struct mount_point*);

    // Check that we can unmount the device
    if ((code = vfs_can_unmount(point)) != K_OKAY);
        goto end;
    
    // Remove the device from the list and deallocate the device's structure
    kfree(point);
    dynlist_remove(mounts, device_id);

    goto end;
invalid_dev:
    code = K_INVDR;
end:
    spinlock_release(&vfs_lock);
    return code;
}

int vfs_unmount_mount_point(const char *mount_point)
{
    int code = K_OKAY;
    int64_t device_id;
    struct mount_point *point;
    spinlock_acquire(&vfs_lock);

    // Check that we have this device mounted
    if ((device_id = vfs_get_mount(mount_point)) == -1)
        goto invalid_dev;
    point = dynlist_get(mounts, device_id, struct mount_point*);

    // Check that we can unmount the device
    if ((code = vfs_can_unmount(point)) != K_OKAY);
        goto end;
    
    // Remove the device from the list and deallocate the device's structure
    kfree(point);
    dynlist_remove(mounts, device_id);

    goto end;
invalid_dev:
    code = K_INVDR;
end:
    spinlock_release(&vfs_lock);
    return code;
}

device_handle vfs_open(const char *path)
{
    int64_t device_id;
    device_handle handle = last_handle++;
    struct mount_point *point;
    spinlock_acquire(&vfs_lock);

    // If the path starts with /dev, we're trying to open the device itself.
    // Search for the device in mount points
    if ((device_id = vfs_get_mount(path)) == -1)
        goto invalid;
    point = dynlist_get(mounts, device_id, struct mount_point*);

    // Try to assign the handle to the device
    if (vfs_assign_handle(point, handle) != K_OKAY)
        // TODO: more verbose result
        goto invalid;
    
    // Increase the refs counter and return assigned handle
    point->opened_refs++;

    goto end;
invalid:
    handle = K_INVDR;
end:
    spinlock_release(&vfs_lock);
    return handle;
}

int vfs_read(device_handle handle, uintptr_t at, size_t size, void *buffer)
{

}

int vfs_write(device_handle handle, uintptr_t at, size_t size, void *buffer)
{

}

int vfs_close(device_handle handle)
{

}
