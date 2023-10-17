#include <kernel/kernel.h>
#include <kernel/include/C/math.h>

#include <drivers/fs/fat/fat32.h>

struct fat32 *fat32_make(struct devfs_node *block_device)
{
    uint8_t buffer[512];
    struct fat32 *fat = kmalloc(sizeof(struct fat32));

    // read both BIOS Parameter Block and Extended Boot Record from the disk into buffer
    devfs_block_read(block_device, 0, 1, &buffer[0]);

    // copy values from buffer to the fat structure
    memcpy(&fat->bpb, buffer, sizeof(struct fat32_bpb));
    memcpy(&fat->ebr, &buffer[sizeof(struct fat32_bpb)], sizeof(struct fat32_ebr));

    // validate signature of the EBR
    if (fat->ebr.signature != 0x28 && fat->ebr.signature != 0x29)
    {
        printk(KERN_INFO "fat32[%s]: EBR invalid signature.", block_device->path);
        fat32_cleanup(fat);
        return NULL;
    }
    
    // read the fsinfo into buffer and copy to structure
    devfs_block_read(block_device, fat->ebr.fsinfo_sector, 1, &buffer[0]);
    memcpy(&fat->fsinfo, buffer, sizeof(struct fat32_fsinfo));

    // validate signatures of fsinfo
    if (fat->fsinfo.lead_signature != 0x41615252 ||
        fat->fsinfo.signature != 0x61417272 ||
        fat->fsinfo.trail_signature != 0xAA550000)
    {
        printk(KERN_INFO "fat32[%s]: fsinfo invalid signature.", block_device->path);
        fat32_cleanup(fat);
        return NULL;
    }


    printk(KERN_DEBUG "fat32[%s]: oem = %8s, label = '%11s'", block_device->path, fat->bpb.oem_id, fat->ebr.label);

    return fat;
}

void fat32_cleanup(struct fat32 *fat)
{
    // TODO: save everything firstly
    kfree(fat);
}
