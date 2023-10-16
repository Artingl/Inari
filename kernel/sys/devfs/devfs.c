#include <kernel/kernel.h>
#include <kernel/sys/devfs/devfs.h>
#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>

#include <drivers/ata/ata_pio.h>

struct devfs_nodes devfs_nodes[DEVFS_NODES_MAX];

int devfs_init()
{
    int r = DEVFS_SUCCESS;
    size_t i;

    // initialize array of nodes
    for (i = 0; i < DEVFS_NODES_MAX; i++)
    {
        devfs_nodes[i].count = 0;
        devfs_nodes[i].nodes = NULL;
        devfs_nodes[i].nodes_type = i;
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

size_t devfs_add_node(uint32_t node_type, struct devfs_node *node, const char *path)
{
    struct devfs_nodes *nodes_list = &devfs_nodes[node_type];
    size_t node_index = nodes_list->count;
    const char *new_path;

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

    // make path for the node
    new_path = devfs_make_path(node->type, node_index, path);

    // save node data inside global list
    memcpy(&nodes_list->nodes[node_index], node, sizeof(struct devfs_node));
    memcpy(&nodes_list->nodes[node_index].path, new_path, min(strlen(path), 256));

    printk(KERN_INFO "devfs: added node %s with type %d at index %d", new_path, node->driver_type, node_index);

    return node_index;
}

void devfs_remove_node(uint32_t node_type, size_t node_index)
{
}

int devfs_block_call_driver(
    struct devfs_node *block_device,
    uint8_t operation,
    uint32_t offset,
    uint32_t n,
    uint8_t *buffer)
{
    if (block_device->driver_type == DEVFS_DRIVER_ATA)
    {
        // call the ATA driver for this block device
        if (operation == DEVFS_BLOCK_WRITE)
        {
            if (ata_pio_write28((struct ata_pio_drive *)block_device->driver, offset, n, (uint16_t*)buffer) != ATA_SUCCESS)
                goto drv_error;
        }
        else if (operation == DEVFS_BLOCK_READ)
        {
            if (ata_pio_read28((struct ata_pio_drive *)block_device->driver, offset, n, (uint16_t*)buffer) != ATA_SUCCESS)
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

int devfs_block_write(struct devfs_node *block_device, uint32_t offset, uint32_t n, uint8_t *buffer)
{
    return devfs_block_call_driver(block_device, DEVFS_BLOCK_WRITE, offset, n, buffer);
}

int devfs_block_read(struct devfs_node *block_device, uint32_t offset, uint32_t n, uint8_t *buffer)
{
    return devfs_block_call_driver(block_device, DEVFS_BLOCK_READ, offset, n, buffer);
}

struct devfs_node *devfs_get_node(const char *path)
{
    size_t i, j, k;

    // search thru nodes
    for (i = 0; i < DEVFS_NODES_MAX; i++)
    {
        struct devfs_nodes *nodes_list = &devfs_nodes[i];

        for (j = 0; j < nodes_list->count; j++)
        {
            struct devfs_node *node = &nodes_list->nodes[j];

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
    if (type >= DEVFS_NODES_MAX)
        return NULL;

    return &devfs_nodes[type];
}

const char *devfs_make_path(uint32_t node_type, size_t index, const char *reference)
{
    static char REFERENCES_BUFFER[512];
    size_t i, inc_index;

    // Check if we have '*' signs in the reference. If we don't, just return it back.
    for (i = 0; i < strlen(reference); i++)
    {
        if (reference[i] == '*')
        {
            inc_index = i;
            goto has_inc;
        }
    }

    // didn't found any '*' signs
    return reference;
has_inc:
    // copy the reference to the buffer
    memcpy(&REFERENCES_BUFFER[0], &reference[0], inc_index);

    // add index inside the reference
    if (node_type == DEVFS_NODE_BLOCK_DEVICE)
    {
        // parse index as alphabetical
        while (index != 0)
        {

            // storing remainder in octal array
            REFERENCES_BUFFER[inc_index++] = index % 8 + 'a';
            index /= 8;
        }
    }

    // add NULL-terminator
    REFERENCES_BUFFER[inc_index++] = '\0';

    return &REFERENCES_BUFFER[0];
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

    size_t i, counter;
    int r;

    struct devfs_node node;
    node.type = DEVFS_NODE_BLOCK_DEVICE;

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

                devfs_add_node(DEVFS_NODE_BLOCK_DEVICE, &node, "/dev/hda*");
                counter++;
            }
        }
    }

    // TODO: initialize more drives

    return DEVFS_SUCCESS;
}
