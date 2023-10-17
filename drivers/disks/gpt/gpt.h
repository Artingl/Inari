#pragma once

#include <kernel/include/C/typedefs.h>

// TODO: I don't like that filesystems/GPT/MBR drivers would be directly accessing the block device.
//       Implement different way of doing it.
#include <kernel/sys/devfs/devfs.h>

#define GPT_MAX_PARTITIONS 128

enum
{
    GPT_SUCCESS = 0,
    GPT_INVALID = 1,
    GPT_UNUSED = 2,
};

// Protective Master Boot Record
struct gpt_pmbr
{
    uint8_t boot_indicator;
    uint8_t starting_chs[3];
    uint8_t os_type;
    uint8_t ending_chs[3];
    uint32_t starting_lba;
    uint32_t ending_lba;
} __attribute__((packed));

// Partition Table Header
struct gpt_pth
{
    uint64_t signature;
    uint32_t gpt_revision;
    uint32_t header_size;
    uint32_t crc32_checksum;
    uint32_t reserved0;
    uint64_t lba_of_this_header;
    uint64_t lba_of_alternate_header;
    uint64_t first_usable_block;
    uint64_t last_usable_block;
    uint8_t guid[16];
    uint64_t starting_lba_of_guid_array;
    uint32_t number_of_partitions;
    uint32_t size_of_array_entry;
    uint32_t crc32_of_array;
    // uint8_t reserved1[92 - 0x5c];
} __attribute__((packed));

struct gpt_partition_entry
{
    uint8_t partition_type[16];
    uint8_t guid[16];
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    uint8_t partition_name[72];
} __attribute__((packed));

struct gpt_info
{
    struct gpt_pmbr pmbr;
    struct gpt_pth pth;

    struct gpt_partition_entry partitions[GPT_MAX_PARTITIONS];
};

int gpt_read(struct devfs_node *block_device, struct gpt_info *gpt);
int gpt_check_entry(struct gpt_partition_entry *entry);
int gpt_validate(struct gpt_info *gpt);

struct gpt_partition_entry *gpt_read_partition(
    struct devfs_node *block_device,
    size_t partition,
    struct gpt_info *gpt);
