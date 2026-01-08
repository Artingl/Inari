#ifndef _INARI_VFS_H
#define _INARI_VFS_H

#include <misc/types.h>
#include <misc/list.h>

#include <kernel/sys/block.h>

#define	VFS_READ			0x01
#define	VFS_WRITE			0x02
#define	VFS_OPEN_EXISTING	0x00
#define	VFS_CREATE_NEW		0x04
#define	VFS_CREATE_ALWAYS	0x08
#define	VFS_OPEN_ALWAYS		0x10
#define	VFS_OPEN_APPEND		0x30

#define VFS_STAT_FILE       (1 << 0)
#define VFS_STAT_DIR        (1 << 1)
#define VFS_STAT_BLOCK      (1 << 2)
#define VFS_STAT_CHAR       (1 << 3)

struct vfs_node;
struct vfs_mount_point;
struct vfs_layer;

typedef int64_t vfs_handle_t;

struct vfs_node {
    char name[CONFIG_VFS_NAME_MAX];
    uint32_t st_mode;           // File type + permissions
    uint64_t size;              // File size (bytes)

    size_t off;                 // Node offset in the directory
};

struct vfs_layer_ops {
    int (*open)(struct vfs_mount_point *mount, vfs_handle_t *handle, const char *path, int flags);
    int (*close)(struct vfs_mount_point *mount, vfs_handle_t handle);
    int (*read)(struct vfs_mount_point *mount, vfs_handle_t handle, void *buf, size_t len, size_t *rlen);
    int (*write)(struct vfs_mount_point *mount, vfs_handle_t handle, const void *buf, size_t sz);
    int (*seek)(struct vfs_mount_point *mount, vfs_handle_t handle, size_t offset);
    int (*tell)(struct vfs_mount_point *mount, vfs_handle_t handle, size_t *offset);
    int (*size)(struct vfs_mount_point *mount, vfs_handle_t handle, size_t *size);
    int (*readdir)(struct vfs_mount_point *mount, const char *path, struct vfs_node *node, size_t offset);
    // int (*ioctl)(struct vfs_mount_point *mount, unsigned long req, void *arg);
};

struct vfs_layer
{
    int (*mount)(struct vfs_mount_point*);
    int (*unmount)(struct vfs_mount_point*);

    const char *name;
    struct vfs_layer_ops *ops;

    struct list_head list;
};

struct vfs_mount_point
{
    const char *fs_name; // e.g. FAT32, EXT2
    const char *mount_point;

    dev_t bdev;

    void *fs_data;
    struct vfs_layer *layer;

    struct list_head list;
};

int vfs_init();

int vfs_mount(dev_t dev, const char* path);
int vfs_unmount(const char* path);

int vfs_add_layer(struct vfs_layer *layer);
int vfs_remove_layer(struct vfs_layer *layer);

int vfs_open(vfs_handle_t *file, const char *path, int flags);
int vfs_close(vfs_handle_t handle);
int vfs_seek(vfs_handle_t handle, size_t offset);
int vfs_tell(vfs_handle_t handle, size_t *offset);
int vfs_size(vfs_handle_t handle, size_t *size);
int vfs_read(vfs_handle_t handle, void *buf, size_t len, size_t *rlen);
int vfs_readdir(const char *path, struct vfs_node *node);
int vfs_write(vfs_handle_t handle, const void *buf, size_t sz);

int vfs_alloc_handle(struct vfs_mount_point *mount, vfs_handle_t *handle, void *data);
void *vfs_handle_data(vfs_handle_t handle);
void vfs_kill_handle(vfs_handle_t handle);

#endif