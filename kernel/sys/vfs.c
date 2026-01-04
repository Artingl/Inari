#include <kernel/inari.h>
#include <kernel/sys/block.h>
#include <kernel/sys/vfs.h>
#include <kernel/sys/stat.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/errno.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/format.h>

LIST_HEAD(vfs_nodes);

static int vfs_block_read(struct vfs_node *node, void *buf, size_t len, size_t offset) {}
static int vfs_block_write(struct vfs_node *node, const void *buf, size_t len, size_t offset) {}
static int vfs_block_open(struct vfs_node *node) {}
static int vfs_block_close(struct vfs_node *node) {}
static int vfs_block_readdir(struct vfs_node *node, struct vfs_node **out) {}
static int vfs_block_ioctl(struct vfs_node *node, unsigned long req, void *arg) {}

static struct vfs_ops vfs_block_ops = {
    .read    = &vfs_block_read,
    .write   = &vfs_block_write,
    .open    = &vfs_block_open,
    .close   = &vfs_block_close,
    .readdir = &vfs_block_readdir,
    .ioctl   = &vfs_block_ioctl
};

int vfs_init()
{

    return 0;
}

int vfs_register_blkdev(dev_t dev, struct vfs_ops *ops)
{}

int vfs_unregister_blkdev(dev_t dev)
{}


// int vfs_register_block_device(struct block_device *bdev, const char *name)
// {
//     struct vfs_node *node = kmalloc(sizeof(*node));
//     if (!node) return -EINVAL;

//     strcpy(node->name, name);
//     node->st_mode = S_IFBLK | 0600;
//     node->size    = bdev->size;
//     node->ops     = &vfs_block_ops;
//     node->data    = bdev;

//     return 0;
// }

// int vfs_unregister_block_device(struct vfs_node *node)
// {
//     if (!node) return -EINVAL;
//     list_del(&node->children);
//     kfree(node);
//     return 0;
// }
