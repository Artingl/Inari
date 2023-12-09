#pragma once

#include <kernel/include/C/typedefs.h>

#include <drivers/disks/device_wrapper.h>

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

#define FAT32_READ_ONLY 0x01
#define FAT32_HIDDEN 0x02
#define FAT32_SYSTEM 0x04
#define FAT32_VOLUME_ID 0x08
#define FAT32_DIRECTORY 0x10
#define FAT32_ARCHIVE 0x20
#define FAT32_LFN (FAT32_READ_ONLY | FAT32_HIDDEN | FAT32_SYSTEM | FAT32_VOLUME_ID)

struct fat32_file
{
    uint8_t name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t high_cluster_number;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_custer_number;
    uint32_t file_size;
} __attribute__((packed));

struct fat32_lfn
{
    uint8_t order;
    uint8_t first_chars[10];
    uint8_t attribute;
    uint8_t entry_type;
    uint8_t checksum;
    uint8_t second_chars[12];
    uint16_t zero;
    uint8_t third_chars[4];
} __attribute__((packed));

union fat32_entry
{
    uint8_t type;

    struct fat32_lfn lfn;

    uint8_t __[32];
} __attribute__((packed));

struct fat32_info
{
    // the target disk
    struct driver_disk disk_device;

    struct fat32_bpb bpb;
    struct fat32_ebr ebr;
    struct fat32_fsinfo fsinfo;

    uint32_t first_data_sector;
    uint8_t read_only;
};

enum FAT32_STATUS
{
    FAT32_SUCCESS = 0,
    FAT32_INVALID = 1, // not a fat partition

    FAT32_EMPTY = 2,
};

int fat32_load(struct fat32_info *fat, struct driver_disk disk);
void fat32_cleanup(struct fat32_info *fat);

int fat32_parse_dir(struct fat32_info *fat, uint32_t cluster);
void fat32_long_name(struct fat32_info *fat, char *buffer);