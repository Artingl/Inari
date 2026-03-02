#ifdef CONFIG_DRV_GPT
#include <kernel/errno.h>
#include <kernel/event.h>
#include <kernel/kprintf.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/module.h>
#include <kernel/sys/block.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

#include <misc/string.h>

#define GPT_UNUSED_ENTRY "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
#define GPT_MAGIC        "EFI PART"

struct gpt_header {
    char magic[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t __;
    uint64_t curr_lba;
    uint64_t bak_lba;
    uint64_t first_usbl_lba, last_usbl_lba;
    char guid[16];
    uint64_t entries_lba;
    uint32_t entries_cnt;
    uint32_t entry_sz;
} __attribute__((packed));

struct gpt_entry {
    char guid[16];
    char unique_guid[16];
    uint64_t start_lba;
    uint64_t end_lba;
    uint64_t attributes;
    char name[72];
} __attribute__((packed));

typedef struct {
    struct gpt_header header;
    struct gpt_entry entry;
    dev_t disk_bdev;
} gpt_partition_t;

static char *helper_utf16le_to_ascii(char *utf16le_str, size_t len) {
    if (len % 2 != 0)
        return NULL;
    size_t ascii_len = len / 2;
    for (size_t i = 0, j = 0; i < len; i += 2, j++) {
        utf16le_str[j] = (char)utf16le_str[i];
    }
    utf16le_str[ascii_len] = '\0';
    return utf16le_str;
}

static int gpt_read_blocks(struct device *bdev, uint64_t lba, void *buf, size_t nblocks) {
    gpt_partition_t *partition = (gpt_partition_t *)bdev->driver_data;
    if (!partition)
        return -ENODEV;
    struct device *disk_bdev = block_get(partition->disk_bdev);
    if (!bdev)
        return -ENODEV;
    /* Check that we don't exceed the partition boundaries */
    if (lba > partition->entry.end_lba - partition->entry.start_lba)
        return -EINVAL;
    return ((struct block_ops *)disk_bdev->ops)->read_blocks(disk_bdev, lba + partition->entry.start_lba, buf, nblocks);
}

static int gpt_write_blocks(struct device *bdev, uint64_t lba, const void *buf, size_t nblocks) {
    gpt_partition_t *partition = (gpt_partition_t *)bdev->driver_data;
    if (!partition)
        return -ENODEV;
    struct device *disk_bdev = block_get(partition->disk_bdev);
    if (!bdev)
        return -ENODEV;
    /* Check that we don't exceed the partition boundaries */
    if (lba > partition->entry.end_lba - partition->entry.start_lba)
        return -EINVAL;
    return ((struct block_ops *)disk_bdev->ops)
        ->write_blocks(disk_bdev, lba + partition->entry.start_lba, buf, nblocks);
}

static struct block_ops gpt_block_ops = {.read_blocks = &gpt_read_blocks, .write_blocks = &gpt_write_blocks};

static void handle_block_dev_load(dev_t dev) {
    /* We only need block devices under disks group */
    if (!ISGROUP(DRVID(dev), DRIVER_DISKS_GROUP))
        return;

    gpt_partition_t *partition;
    struct gpt_entry entry;
    struct gpt_header header;
    struct device *bdev = block_get(dev);
    uint8_t *lba = (uint8_t *)kmalloc(bdev->group->block_size);
    size_t offset = 0, lba_offset = 0, i, partitions = 0;

    if (!bdev || !((struct block_ops *)bdev->ops)->read_blocks)
        goto end;

    if (((struct block_ops *)bdev->ops)->read_blocks(bdev, 1, (void *)&lba[0], 1) != 0) {
        lba_offset = 1;
        goto read_err;
    }
    memcpy((void *)&header, &lba[0], sizeof(header));

    /* Check magic */
    if (memcmp(GPT_MAGIC, (void *)&header.magic[0], 8) != 0)
        goto end;

    /* Read lba of partition entries */
    if (((struct block_ops *)bdev->ops)->read_blocks(bdev, header.entries_lba, (void *)&lba[0], 1) != 0) {
        lba_offset = header.entries_lba;
        goto read_err;
    }
    memcpy((void *)&entry, &lba[0], sizeof(entry));

    /* Parse all partition entries */
    for (i = 0; i < header.entries_cnt; i++) {
        /* Check that the partition entry is not unused */
        if (memcmp(&entry.guid[0], GPT_UNUSED_ENTRY, 16) == 0)
            goto next;
        helper_utf16le_to_ascii(&entry.name[0], 36);
        partition = (gpt_partition_t *)kmalloc(sizeof(gpt_partition_t));
        partition->entry = entry;
        partition->header = header;
        partition->disk_bdev = dev;

        register_blkdev(GPT_DRIVER, &gpt_block_ops, (entry.end_lba - entry.start_lba) * bdev->group->block_size,
                        (void *)partition, NULL);

        partitions++;
#ifdef CONFIG_DEBUG
        kprintf("gpt: found partition '%s' on dev:blk_%s%d; size %uKiB", entry.name, bdev->group->name, DEVID(dev),
                (entry.end_lba - entry.start_lba) * bdev->group->block_size / 1024);
#endif

    next:
        /* Ensure we don't overrun lba buffer */
        if (offset + header.entry_sz >= bdev->group->block_size) {
            /* Request more info */
            lba_offset++;
            if (((struct block_ops *)bdev->ops)
                    ->read_blocks(bdev, header.entries_lba + lba_offset, (void *)&lba[0], 1) != 0) {
                lba_offset = header.entries_lba + lba_offset;
                goto read_err;
            }

            offset = 0;
        } else
            offset += header.entry_sz;
        memcpy((void *)&entry, &lba[offset], sizeof(entry));
    }

    kprintf("gpt: found %u partitions on dev:blk_%s%d", partitions, bdev->group->name, DEVID(dev));
    goto end;
read_err:
    kprintf("gpt: failed to read lba%d for dev:blk_%s%d", lba_offset, bdev->group->name, DEVID(dev));
end:
    kfree((void *)lba);
}

static int gpt_probe() {
    register_blkdev_group(GPT_DRIVER, 512, "gpt");

    /* During initial GPT module load, we analyze all loaded block devs.
     * It is possible that some block devs were loaded before GPT module
     * itself was loaded. */
    dev_t devs[128];
    int offset = 0, count = 0;
    while ((count = block_get_refs(&devs[0], offset, 128)) > 0) {
        for (; count > 0; count--)
            handle_block_dev_load(devs[count - 1]);
        offset += 128;
    }

    return 0;
}

static int gpt_event_handler(event_t event) {
    switch (event.type) {
    case EVENT_LOAD_BLKDEV:
        handle_block_dev_load(event.as.dev);
    }

    return EVENT_HANDLED;
}

/* TODO: when we'll deal with cleaning up in future, dont forget to kfree the gpt_partition_t we allocated above */

module_t gpt_module = {
    .probe = gpt_probe,
    .event_bus = gpt_event_handler,
};

module_register("gpt", gpt_module);

#endif