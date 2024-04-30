#include <kernel/kernel.h>
#include <kernel/include/C/math.h>

#include <drivers/fs/ext2/ext2.h>

int ext2_load(struct ext2_info *ext, struct driver_disk disk)
{
    double v0, v1;
    ext->disk_device = disk;

    // read superblock
    ext->disk_device.read_handler(
        ext->disk_device.device,
        EXT2_SUPERBLOCK_LBA, 2, // superblock is exactly 2 LBAs in size (1024 bytes)
        (uint8_t*)&ext->superblock);
    if (ext->superblock.signature != EXT2_SIGNATURE)
    {
        printk("ext2: invalid signature 0x%2x", ext->superblock.signature);
        ext2_cleanup(ext);
        return EXT2_INVALID;
    }

    // check filesystem errors
    if (ext->superblock.on_error == EXT2_ERROR_PANIC_KERNEL)
        panic("ext2: superblock filesystem error %d", ext->superblock.on_error);
    if (ext->superblock.on_error == EXT2_ERROR_REMOUNT_AS_READONLY)
        ext->read_only = 1;

    // check if this fs has extended superblock
    if (EXT2_EXTENDED(ext))
    {
        // check if this fs has required features that we don't support right now
        if (ext->superblock.extended.required_features & EXT2_REQUIRED_COMPRESSION ||
            ext->superblock.extended.required_features & EXT2_REQUIRED_RELAY_JOURNAL ||
            ext->superblock.extended.required_features & EXT2_REQUIRED_JOURNAL_DEV)
        {
            printk(KERN_WARNING "ext2: required feature not implemented 0x%x",
                   (unsigned long)ext->superblock.extended.required_features);
            ext2_cleanup(ext);
            return EXT2_INVALID;
        }

        // print some debug info
        printk("ext2: last_mount = '%s', required_features = 0x%x, optional_features = 0x%x",
               ext->superblock.extended.last_mount_path,
               (unsigned long)ext->superblock.extended.required_features,
               (unsigned long)ext->superblock.extended.optional_features);
    }

    // count block groups
    v0 = round(ext->superblock.total_blocks_bg / ext->superblock.total_blocks);
    v1 = round(ext->superblock.total_inodes_bg / ext->superblock.total_inodes);
    ext->block_groups_count = v0 / v1;

    printk("ext2: block_groups = %d");

    // read block groups table
    ext->block_groups = kcalloc(ext->block_groups_count, sizeof(struct ext2_block));
    ext->disk_device.read_handler(
        ext->disk_device.device,
        EXT2_SUPERBLOCK_LBA, 2, // superblock is exactly 2 LBAs in size (1024 bytes)
        (uint8_t*)ext->block_groups);

    // TODO: we must support EXT2_REQUIRED_TYPE_FIELD for the dummy_gpt.img partition 1 to work
    return EXT2_SUCCESS;
}

void ext2_cleanup(struct ext2_info *ext)
{
    // flush data onto the partition

    kfree(ext->block_groups);
}
