#pragma once

#include <kernel/include/C/typedefs.h>

#define FS_DIRECTORY 0x00
#define FS_FILE 0x01

#define FS_FAT32 0x00

struct fs_entry
{
};

struct fs_info
{
    uint8_t filesystem_id;

    struct fs_entry *entries;
    size_t entries_count;
};
