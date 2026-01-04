#ifndef _INARI_VFS_H
#define _INARI_VFS_H

#include <misc/types.h>
#include <misc/list.h>

struct vfs_node;

struct vfs_ops {
    int (*read)(struct vfs_node *node, void *buf, size_t len, size_t offset);
    int (*write)(struct vfs_node *node, const void *buf, size_t len, size_t offset);
    int (*open)(struct vfs_node *node);
    int (*close)(struct vfs_node *node);
    int (*readdir)(struct vfs_node *node, struct vfs_node **out);
    int (*ioctl)(struct vfs_node *node, unsigned long req, void *arg);
};

struct vfs_node {
    char name[CONFIG_VFS_NAME_MAX];
    uint32_t inode;
    uint32_t st_mode;           // File type + permissions
    uint64_t size;              // File size (bytes)
    struct vfs_ops *ops;        // File operations
    void *data;                 // FS-specific data

    struct vfs_node *parent;
    struct list_head children;  // If directory
};

int vfs_init();

// extern int vfs_register_blkdev(dev_t dev, struct vfs_ops *ops);
// extern int vfs_unregister_blkdev(dev_t dev);

#endif