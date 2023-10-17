#pragma once

#include <kernel/include/C/typedefs.h>

// TODO: I don't like that filesystems/GPT/MBR drivers would be directly accessing the block device.
//       Implement different way of doing it.
#include <kernel/sys/devfs/devfs.h>

struct fat32_bpb
{
    uint8_t jmp[3];
    uint8_t oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fats_count;
    uint16_t root_dir_count;
    uint16_t total_sectors0;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat; // FAT12/FAT16 only, ignore this
    uint16_t sectors_per_track;
    uint16_t head_side_on_media;
    uint32_t hidden_sectors;
    uint32_t total_sectors1;
} __attribute__((packed));

struct fat32_ebr
{
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t bbs_sector; // bbs - backup boot sector
    uint8_t reserved[12];
    uint8_t driver_number;
    uint8_t flags_nt;
    uint8_t signature;
    uint32_t serial;
    uint8_t label[11];
    uint8_t system_id[8];
    uint8_t boot_code[420];
    uint16_t bios_signature;
} __attribute__((packed));

struct fat32_fsinfo
{
    uint32_t lead_signature;
    uint8_t reserved0[480];
    uint32_t signature;
    uint32_t last_known_cluster;
    uint32_t cluster_start;
    uint8_t reserved1[12];
    uint32_t trail_signature;
} __attribute__((packed));

struct fat32
{
    // the target disk
    struct devfs_node *block_device;

    struct fat32_bpb bpb;
    struct fat32_ebr ebr;
    struct fat32_fsinfo fsinfo;
};

struct fat32 *fat32_make(struct devfs_node *block_device);
void fat32_cleanup(struct fat32 *fat);
