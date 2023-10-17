#include <kernel/kernel.h>
#include <kernel/sys/devfs/devfs.h>

#include <kernel/include/C/math.h>
#include <kernel/include/C/stdlib.h>
#include <kernel/include/C/string.h>

#include <drivers/disks/gpt/gpt.h>
#include <drivers/disks/mbr/mbr.h>
#include <drivers/disks/ata/ata_pio.h>

struct devfs_nodes nodes[DEVFS_NODES_COUNT];

int devfs_init()
{
    int r = DEVFS_SUCCESS;
    size_t i;

    // initialize array of nodes
    for (i = 0; i < DEVFS_NODES_COUNT; i++)
    {
        nodes[i].count = 0;
        nodes[i].nodes = NULL;
        nodes[i].nodes_type = i;
    }

    if ((r = devfs_parse_disks()) != DEVFS_SUCCESS)
        goto end;

    if ((r = devfs_parse_cpu()) != DEVFS_SUCCESS)
        goto end;

    if ((r = devfs_parse_pci()) != DEVFS_SUCCESS)
        goto end;

end:
    return r;
}

size_t devfs_add_node(
    uint32_t node_type,
    struct devfs_node *node,
    uint32_t path_type,
    const char *path)
{
    struct devfs_nodes *nodes_list = &nodes[node_type];
    size_t node_index = nodes_list->count;

    // initialize the nodes array if hasn't already
    if (nodes_list->nodes == NULL)
    {
        nodes_list->count++;
        nodes_list->nodes = kmalloc(sizeof(struct devfs_node));
    }
    else
    {
        // expand the array
        nodes_list->count++;
        nodes_list->nodes = krealloc(nodes_list->nodes, nodes_list->count * sizeof(struct devfs_node));
    }

    // save node data inside global list
    memcpy(&nodes_list->nodes[node_index], node, sizeof(struct devfs_node));

    // make path for the node
    devfs_make_path(
        path_type,
        node_index,
        path,
        &nodes_list->nodes[node_index].path[0]);

    printk(KERN_INFO "devfs: added node '%s' with type %d at index %d",
           nodes_list->nodes[node_index].path, node->driver_type, node_index);

    return node_index;
}

void devfs_block_find_partitions(size_t block_id)
{
    size_t i, child_block_id, partition_index = 0;
    struct devfs_node new_child_block;
    struct devfs_node *block = &nodes[DEVFS_NODE_BLOCK_DEVICE].nodes[block_id];
    struct devfs_node *child_block_ptr;
    struct gpt_info gpt;

    // set parameters for new block device based on the parent block
    new_child_block.type = DEVFS_NODE_BLOCK_DEVICE;
    new_child_block.driver_type = block->driver_type;
    new_child_block.driver = block->driver;
    new_child_block.parent = block;

    // make new path for the child
    char child_path[DEVFS_PATH_MAX];
    memcpy(&child_path[0], block->path, DEVFS_PATH_MAX);
    child_path[strlen(block->path)] = '*';

    if (gpt_read(block, &gpt) == GPT_SUCCESS)
    {
        // We found GPT on the disk, allocate partitions for in in devfs
        for (i = 0; i < GPT_MAX_PARTITIONS; i++)
        {
            struct gpt_partition_entry *partition = &gpt.partitions[i];

            if (gpt_check_entry(partition) == GPT_SUCCESS)
            {
                // save some metadata for this block device
                new_child_block.metadata.disk.start_lba = partition->starting_lba;
                new_child_block.metadata.disk.end_lba = partition->ending_lba;
                new_child_block.metadata.disk.total_sectors = partition->ending_lba - partition->starting_lba;

                // add the child node
                child_block_id = devfs_add_node(
                    DEVFS_NODE_BLOCK_DEVICE, &new_child_block,
                    DEVFS_NUMERIC_INDEX, child_path);
            }
        }
    }
}

void devfs_remove_node(uint32_t node_type, size_t node_index)
{
}

int devfs_block_call_driver(
    struct devfs_node *block_device,
    uint8_t operation,
    uint64_t offset,
    uint64_t n,
    uint8_t *buffer)
{
    if (block_device->driver_type == DEVFS_DRIVER_ATA)
    {
        if (offset + n > block_device->metadata.disk.total_sectors)
            return DEVFS_OUT_OF_BOUNDS;

        // call the ATA driver for this block device
        if (operation == DEVFS_BLOCK_WRITE)
        {
            if (ata_pio_write28(
                    (struct ata_pio_drive *)block_device->driver,
                    block_device->metadata.disk.start_lba + offset, n,
                    (uint8_t *)buffer) != ATA_SUCCESS)
                goto drv_error;
        }
        else if (operation == DEVFS_BLOCK_READ)
        {
            if (ata_pio_read28(
                    (struct ata_pio_drive *)block_device->driver,
                    block_device->metadata.disk.start_lba + offset, n,
                    (uint8_t *)buffer) != ATA_SUCCESS)
                goto drv_error;
        }
        else
        {
            // invalid operation
            goto err;
        }

        goto success;
    }

    return DEVFS_INVALID_DRIVER;
success:
    return DEVFS_SUCCESS;
err:
    return DEVFS_UNSPECIFIED_ERROR;
drv_error:
    return DEVFS_DRIVER_ERROR;
}

int devfs_block_write(struct devfs_node *block_device,
                      uint64_t offset,
                      uint64_t n,
                      uint8_t *buffer)
{
    return devfs_block_call_driver(block_device, DEVFS_BLOCK_WRITE, offset, n, buffer);
}

int devfs_block_read(struct devfs_node *block_device,
                     uint64_t offset,
                     uint64_t n,
                     uint8_t *buffer)
{
    return devfs_block_call_driver(block_device, DEVFS_BLOCK_READ, offset, n, buffer);
}

struct devfs_node *devfs_get_node(const char *path)
{
    size_t i, j, k;

    // search thru nodes
    for (i = 0; i < DEVFS_NODES_COUNT; i++)
    {
        struct devfs_nodes *nodes_list = &nodes[i];

        for (j = 0; j < nodes_list->count; j++)
        {
            struct devfs_node *node = &nodes_list->nodes[j];

            printk(" -- '%s' == '%s'", node->path, path);

            if (strcmp(node->path, path) == 0)
            {
                // we found the node!
                return node;
            }
        }
    }

    return NULL;
}

struct devfs_nodes *devfs_get_nodes(uint32_t type)
{
    if (type >= DEVFS_NODES_COUNT)
        return NULL;

    return &nodes[type];
}

size_t devfs_make_path(
    uint32_t index_type,
    size_t index,
    const char *reference,
    char *result)
{
    size_t i, i_buffer, ref_len = strlen(reference);

    // fill the result buffer with NULL
    memset(result, 0, DEVFS_PATH_MAX);

    // Check if we have '*' signs in the reference. If we don't, just return it back.
    for (i = 0; i < ref_len; i++)
    {
        if (reference[i] == '*')
        {
            i_buffer = i;
            goto has_inc;
        }
    }

    // didn't found any '*' signs
    memcpy(result, reference, ref_len);
    return i;
has_inc:
    // copy the reference to the buffer
    memcpy(result, reference, i_buffer);

    if (index_type == DEVFS_ALPHABETIC_INDEX)
    {
        if (index == 0)
            result[i_buffer++] = 'a';

        // parse index as alphabetical
        while (index != 0)
        {
            // storing remainder in octal array
            result[i_buffer++] = (index % 8) + 'a';
            index /= 8;
        }
    }
    else
    {
        // parse as numerical
        itoa(index, &result[i_buffer++], 10);
    }

    return i_buffer;
}

int devfs_parse_cpu()
{
    return DEVFS_SUCCESS;
}

int devfs_parse_pci()
{
    return DEVFS_SUCCESS;
}

int devfs_parse_disks()
{

    size_t i, block_id, counter;
    int r;

    struct devfs_node node;
    node.type = DEVFS_NODE_BLOCK_DEVICE;
    node.parent = NULL;

    struct ata_pio_drive const *ata_drives;
    struct ata_pio_drive *ata_drive;

    // Detect all disks on the system
    r = ata_pio_init();
    if (r != ATA_NO_DRIVES_FOUND)
    {
        // We were able to found some ATA drives.
        //
        // Note: The amount of ATA disks is hardcoded (4 disks only can be found by the driver).
        //       As far as I know, the maximum amount of ATA disks that can be on the system is 4,
        //       since there are 2 controllers and one controller can deal only with 2 disks.
        //       So this should not be a problem for us, but make it not hardcoded some time in the future.
        ata_drives = ata_pio_drives();
        counter = 0;

        for (i = 0; i < 4; i++)
        {
            ata_drive = &ata_drives[i];

            // the driver successfully initialized the drive
            if (ata_drive->present)
            {
                // setup the node
                node.driver_type = DEVFS_DRIVER_ATA;
                node.driver = ata_drive;
                node.metadata.disk.start_lba = ata_drive->start_lba;
                node.metadata.disk.end_lba = ata_drive->end_lba;
                node.metadata.disk.total_sectors = ata_drive->end_lba - ata_drive->start_lba;

                // add the disk and related partitions as block device(s)
                block_id = devfs_add_node(
                    DEVFS_NODE_BLOCK_DEVICE, &node,
                    DEVFS_ALPHABETIC_INDEX, "/dev/hd*");
                devfs_block_find_partitions(block_id);
                counter++;
            }
        }
    }

    // TODO: initialize more drives

    return DEVFS_SUCCESS;
}
