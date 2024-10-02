#pragma once

#include <kernel/libc/typedefs.h>
#include <kernel/core/vfs/dev.h>

typedef int64_t device_handle;

int vfs_init();

device_handle vfs_open(const char *path);
int vfs_read(device_handle handle, uintptr_t at, size_t size, void *buffer);
int vfs_write(device_handle handle, uintptr_t at, size_t size, void *buffer);
int vfs_close(device_handle handle);

device_handle vfs_mount(const char *mount_point, struct dev_ref *device);
int vfs_unmount_handle(device_handle handle);
int vfs_unmount_mount_point(const char *mount_point);
