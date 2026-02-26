#ifdef CONFIG_DRV_MBR
#include <kernel/module.h>
#include <kernel/event.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/block.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/errno.h>
#include <kernel/kprintf.h>
#include <kernel/sys/device.h>

#include <misc/string.h>

#define MBR_SIGNATURE 0xaa55

struct mbr_partition_table
{
    uint8_t attr;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba;
    uint32_t secs;
} __attribute__((packed));

struct mbr_header
{
    uint8_t bootstrap[440];
    uint32_t disk_id;
    uint16_t reserved;
    struct mbr_partition_table table0;
    struct mbr_partition_table table1;
    struct mbr_partition_table table2;
    struct mbr_partition_table table3;
    uint16_t signature;
}  __attribute__((packed));

typedef struct
{
    struct mbr_header header;
    struct mbr_partition_table table;
    size_t start_lba;
    size_t end_lba;
    dev_t disk_bdev;
} mbr_partition_t;

static int mbr_read_blocks(struct device *bdev, uint64_t lba, void *buf, size_t nblocks)
{
    mbr_partition_t *partition = (mbr_partition_t*)bdev->driver_data;
    if (!partition) return -ENODEV;
    struct device *disk_bdev = block_get(partition->disk_bdev);
    if (!bdev) return -ENODEV;
    /* Check that we don't exceed the partition boundaries */
    if (lba > partition->end_lba - partition->start_lba) return -EINVAL;
    return ((struct block_ops*)disk_bdev->ops)->read_blocks(disk_bdev, lba + partition->start_lba, buf, nblocks);
}

static int mbr_write_blocks(struct device *bdev, uint64_t lba, const void *buf, size_t nblocks)
{
    mbr_partition_t *partition = (mbr_partition_t*)bdev->driver_data;
    if (!partition) return -ENODEV;
    struct device *disk_bdev = block_get(partition->disk_bdev);
    if (!bdev) return -ENODEV;
    /* Check that we don't exceed the partition boundaries */
    if (lba > partition->end_lba - partition->start_lba) return -EINVAL;
    return ((struct block_ops*)disk_bdev->ops)->write_blocks(disk_bdev, lba + partition->start_lba, buf, nblocks);
}

static struct block_ops mbr_block_ops = {
    .read_blocks = &mbr_read_blocks,
    .write_blocks = &mbr_write_blocks
};

static void register_partition_table(struct device *bdev, struct mbr_header header, struct mbr_partition_table tbl)
{
    if (!(tbl.attr & (1 << 7))) return;

    mbr_partition_t *partition = (mbr_partition_t*)kmalloc(sizeof(mbr_partition_t));

    partition->disk_bdev = bdev->dev;
    partition->table = tbl;
    partition->header = header;
    partition->start_lba = tbl.lba;
    partition->end_lba = tbl.lba + tbl.secs;

    register_blkdev(
        MBR_DRIVER,
        &mbr_block_ops,
        (partition->end_lba - partition->start_lba) * bdev->group->block_size,
        (void*)partition, NULL);

#ifdef CONFIG_DEBUG
        kprintf("mbr: found partition on dev:blk_%s%d; size %uKiB",
            bdev->group->name,
            DEVID(partition->disk_bdev),
            (partition->end_lba - partition->start_lba) * bdev->group->block_size / 1024);
#endif
}

static void handle_block_dev_load(dev_t dev)
{
   /* We only need block devices under disks group */
   if (!ISGROUP(DRVID(dev), DRIVER_DISKS_GROUP)) return;

    struct mbr_header header;
    struct device *bdev = block_get(dev);
    uint8_t *lba = (uint8_t*)kmalloc(bdev->group->block_size);
    size_t lba_offset = 0;

    if (!bdev || !((struct block_ops*)bdev->ops)->read_blocks) goto end;

    if (((struct block_ops*)bdev->ops)->read_blocks(bdev, 0, (void*)&lba[0], 1) != 0)
    {
        lba_offset = 0;
        goto read_err;
    }
    memcpy((void*)&header, &lba[0], sizeof(header));

    if (header.signature != MBR_SIGNATURE) goto end;

    register_partition_table(bdev, header, header.table0);
    register_partition_table(bdev, header, header.table1);
    register_partition_table(bdev, header, header.table2);
    register_partition_table(bdev, header, header.table3);

    goto end;
read_err:
    kprintf("mbr: failed to read lba%d for dev:blk_%s%d", lba_offset, bdev->group->name, DEVID(dev));
end:
    kfree((void*)lba);
}

static int mbr_probe()
{
    register_blkdev_group(MBR_DRIVER, 512, "mbr");

    /* During initial MBR module load, we analyze all loaded block devs.
     * It is possible that some block devs were loaded before MBR module
     * itself was loaded. */
    dev_t devs[128];
    int offset = 0, count = 0;
    while ((count = block_get_refs(&devs[0], offset, 128)) > 0)
    {
        for (; count > 0; count--)
            handle_block_dev_load(devs[count - 1]);
        offset += 128;
    }

    return 0;
}

static int mbr_event_handler(event_t event)
{
    switch (event.type)
    {
        case EVENT_LOAD_BLKDEV:
            handle_block_dev_load(event.as.dev);
    }

    return EVENT_HANDLED;
}

/* TODO: when we'll deal with cleaning up in future, dont forget to kfree the mbr_partition_t we allocated above */

module_t mbr_module = {
    .probe = mbr_probe,
    .event_bus = mbr_event_handler,
};

module_register(
    "mbr",
    mbr_module
);

#endif