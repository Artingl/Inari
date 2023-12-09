#pragma once

#include <kernel/include/C/typedefs.h>

// max path length for the devfs (including NULL)
#define DEVFS_PATH_MAX 256

enum DEVFS_STATUS
{
    DEVFS_SUCCESS = 0,
    DEVFS_UNSPECIFIED_ERROR = 1,
    DEVFS_INVALID_DRIVER = 2,
    DEVFS_DRIVER_ERROR = 3,
    DEVFS_OUT_OF_BOUNDS = 4,
};

enum DEVFS_NODE_TYPES
{
    DEVFS_NODE_BLOCK_DEVICE = 0,
    DEVFS_NODE_CHAR_DEVICE = 1,
    DEVFS_NODE_PIPE = 2,

    DEVFS_NODES_COUNT,
};

enum DEVFS_NODE_DRIVERS
{
    DEVFS_DRIVER_ATA = 0,
};

struct devfs_disk_block_device
{
    uint64_t start_lba;
    uint64_t end_lba;
    uint64_t total_sectors;
};

struct devfs_node
{
    char path[DEVFS_PATH_MAX];

    uint32_t type;        // type of the node
    uint32_t driver_type; // type of the driver that this node uses (can be ignored in some cases)

    // pointer to some driver structure (depends on the type and sometimes driver_type)
    void *driver;

    // parent node of this node (if no parent nodes, then should be set to NULL)
    struct devfs_node *parent;

    // block device specific metadata
    union
    {
        struct devfs_disk_block_device disk;
        // ...
    } metadata;
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

enum
{
    DEVFS_ALPHABETIC_INDEX = 0,
    DEVFS_NUMERIC_INDEX = 1,
};

size_t devfs_make_path(
    uint32_t index_type,
    size_t index,
    const char *reference,
    char *result);

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
    uint64_t offset,
    uint64_t n,
    uint8_t *buffer);

int devfs_block_write(struct devfs_node *block_device,
                      uint64_t offset,
                      uint64_t n,
                      uint8_t *buffer);
int devfs_block_read(struct devfs_node *block_device,
                     uint64_t offset,
                     uint64_t n,
                     uint8_t *buffer);

void devfs_block_find_partitions(size_t block_id);
void devfs_remove_node(uint32_t node_type, size_t node_index);

size_t devfs_add_node(
    uint32_t node_type,
    struct devfs_node *node,
    uint32_t path_type,
    const char *path);
