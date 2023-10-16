#pragma once

#include <kernel/include/C/typedefs.h>

enum
{
    DEVFS_SUCCESS = 0,
    DEVFS_UNSPECIFIED_ERROR = 1,
    DEVFS_INVALID_DRIVER = 2,
    DEVFS_DRIVER_ERROR = 3,
};

enum
{
    DEVFS_NODE_BLOCK_DEVICE = 0,
    DEVFS_NODE_CHAR_DEVICE = 1,
    DEVFS_NODE_PIPE = 2,

    DEVFS_NODES_MAX,
};

enum
{
    DEVFS_DRIVER_ATA = 0,
};

struct devfs_node
{
    char path[256];

    uint32_t type;        // type of the node
    uint32_t driver_type; // type of the driver that this node uses (can be ignored in some cases)

    // pointer to some driver structure (depends on the type and sometimes driver_type)
    void *driver;
};

struct devfs_nodes
{
    size_t count;        // amount of nodes in the list
    uint32_t nodes_type; // type of all nodes inside the list

    struct devfs_node *nodes; // list of nodes
};

int devfs_init();
int devfs_parse_disks();
int devfs_parse_cpu();
int devfs_parse_pci();
// The reference format of the path that's going to be made:
//    For disks, all '*' signs will be replaced with the alphabetical index of the node in list (e.g. /dev/sd* can be /dev/sda, /dev/sdb ... /dev/sdaa, etc.)
//    For other types of devices, the '*' will be replaced with the numerical index of the node in the list (e.g. /dev/cpu/core* can be /dev/cpu/core0, /dev/cpu/core1 ... /dev/cpu/core8, etc.)
//
// Note: The '*' sign also will identify the end of the reference, so no any other data after it would be parsed.
const char *devfs_make_path(uint32_t type, size_t index, const char *reference);

// NULL result for both of them means that nothing was found.
struct devfs_node *devfs_get_node(const char *path);
struct devfs_nodes *devfs_get_nodes(uint32_t type);

enum
{
    DEVFS_BLOCK_WRITE = 0,
    DEVFS_BLOCK_READ = 1,
};

int devfs_block_call_driver(
    struct devfs_node *block_device,
    uint8_t operation,
    uint32_t offset,
    uint32_t n,
    uint8_t *buffer);

int devfs_block_write(struct devfs_node *block_device, uint32_t offset, uint32_t n, uint8_t *buffer);
int devfs_block_read(struct devfs_node *block_device, uint32_t offset, uint32_t n, uint8_t *buffer);

void devfs_remove_node(uint32_t node_type, size_t node_index);
size_t devfs_add_node(uint32_t node_type, struct devfs_node *node, const char *path);