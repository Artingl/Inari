#include <kernel/kernel.h>
#include <kernel/include/C/math.h>

#include <drivers/fs/fat/fat32.h>

#define ENTRIES_PER_SECTOR (SECTOR_SIZE / sizeof(union fat32_entry))
#define SECTOR(fat, cluster) ((((cluster)-2) * fat->bpb.sectors_per_cluster) + fat->bpb.reserved_sectors)

int fat32_make(struct fat32 *fat, struct driver_disk disk)
{
    uint8_t buffer[SECTOR_SIZE];
    fat->disk_device = disk;

    // read both BIOS Parameter Block and Extended Boot Record from the disk into buffer
    fat->disk_device.read_handler(
        fat->disk_device.device, 0, 1,
        &buffer[0]);

    // copy values from buffer to the fat structure
    memcpy(&fat->bpb, buffer, sizeof(struct fat32_bpb));
    memcpy(&fat->ebr, &buffer[sizeof(struct fat32_bpb)], sizeof(struct fat32_ebr));

    // validate signature of the EBR
    if (fat->ebr.signature != 0x28 && fat->ebr.signature != 0x29)
    {
        printk(KERN_INFO "fat32: EBR invalid signature.");
        fat32_cleanup(fat);
        return FAT32_NOT_FAT;
    }

    // read the fsinfo into buffer and copy to structure
    fat->disk_device.read_handler(
        fat->disk_device.device,
        fat->ebr.fsinfo_sector, 1,
        &buffer[0]);
    memcpy(&fat->fsinfo, buffer, sizeof(struct fat32_fsinfo));

    // validate signatures of fsinfo
    if (fat->fsinfo.lead_signature != 0x41615252 ||
        fat->fsinfo.signature != 0x61417272 ||
        fat->fsinfo.trail_signature != 0xAA550000)
    {
        printk(KERN_INFO "fat32: fsinfo invalid signature.");
        fat32_cleanup(fat);
        return FAT32_NOT_FAT;
    }

    int r = fat32_parse_dir(fat, fat->ebr.root_cluster);

    panic("result: %d", r);
    return FAT32_SUCCESS;
}

int fat32_parse_dir(struct fat32 *fat, uint32_t cluster)
{
    union fat32_entry *entry, entries[ENTRIES_PER_SECTOR];
    uint32_t i = 0, sector_offset = 0;

    fat->disk_device.read_handler(
        fat->disk_device.device,
        SECTOR(fat, cluster), 1,
        &entries[0]);
    entry = &entries[i];

    // if we have zero the directory is empty
    if (entry->type == 0x00)
        return FAT32_EMPTY;

    do
    {
        // check that we still have available entries in the array
        if (i > ENTRIES_PER_SECTOR)
        {
            // we exceeded amount of all available entries. read next sector
            i = 0;
            sector_offset++;
            fat->disk_device.read_handler(
                fat->disk_device.device,
                SECTOR(fat, cluster) + sector_offset, 1,
                &entries[0]);
        }

        entry = &entries[i++];
        printk(" -- %u: 0x%x %u %u", i, (unsigned long)entry->type, entries->__[11], entries->lfn.attribute);

        // check if the entry is unused
        if (entry->type == 0xE5)
            continue;

        // check if LFN

        // if ()
    } while (entry->type != 0x00);

    // struct fat32_lfn f;
    // panic("%d %d %d", f.attributes, f.attributes & FAT32_LFN, f.creation_date);

    return FAT32_SUCCESS;
}

void fat32_long_name(struct fat32 *fat, char *buffer)
{
}

void fat32_cleanup(struct fat32 *fat)
{
    // TODO: save everything firstly
}
