#pragma once

#include <kernel/include/C/typedefs.h>

#include <drivers/disks/device_wrapper.h>

#define EXT2_SIGNATURE 0xef53
#define EXT2_SUPERBLOCK_LBA 2

#define EXT2_EXTENDED(ext) (ext->superblock.major_version >= 1)
#define EXT2_READONLY(ext) (ext->read_only)

// These are optional features for an implementation to support,
// but offer performance or reliability gains to implementations that do support them.
enum EXT2_OPTIONAL_FEATURES
{
    EXT2_OPTIONAL_PREALLOC_BLOCKS = 0x0001, // Preallocate some number of (contiguous?) blocks (see byte 205 in the superblock)
                                            // to a directory when creating a new one (to reduce fragmentation?)
    EXT2_OPTIONAL_AFS_INODE = 0x0002,       // AFS server inodes exist
    EXT2_OPTIONAL_FS_JOURNAL = 0x0004,      // File system has a journal (Ext3)
    EXT2_OPTIONAL_INODES_ATTRIB = 0x0008,   // Inodes have extended attributes
    EXT2_OPTIONAL_CAN_RESIZE = 0x0010,      // File system can resize itself for larger partitions
    EXT2_OPTIONAL_HASH_INDEX = 0x0020,      // Directories use hash index
};

// These features if present on a file system are required to be supported by an
// implementation in order to correctly read from or write to the file system.
enum EXT2_REQUIRED_FEATURES
{
    EXT2_REQUIRED_COMPRESSION = 0x0001,   //	Compression is used
    EXT2_REQUIRED_TYPE_FIELD = 0x0002,    //	Directory entries contain a type field
    EXT2_REQUIRED_RELAY_JOURNAL = 0x0004, //	File system needs to replay its journal
    EXT2_REQUIRED_JOURNAL_DEV = 0x0008,   //	File system uses a journal device
};

// These features, if present on a file system, are required in order for an
// implementation to write to the file system, but are not required to read from the file system.
enum EXT2_RO_FEATURES
{
    EXT2_RO_SPARSE_SUPERBLOCKS = 0x0001, //	Sparse superblocks and group descriptor tables
    EXT2_RO_USES_64BIT_SIZE = 0x0002,    //	File system uses a 64-bit file size
    EXT2_RO_DIR_BINARY_TREE = 0x0004,    //	Directory contents are stored in the form of a Binary Tree
};

enum EXT2_FS_STATES
{
    EXT2_STATE_CLEAN = 1,
    EXT2_STATE_HAS_ERRORS = 2,
};

enum EXT2_FS_ERRORS
{
    EXT2_ERROR_IGNORE = 1,
    EXT2_ERROR_REMOUNT_AS_READONLY = 2,
    EXT2_ERROR_PANIC_KERNEL = 3,
};

struct ext2_inode
{
} __attribute__((packed));

struct ext2_block
{
    uint32_t block_address_blk;
    uint32_t block_address_inode;
    uint32_t start_addr_inode_table;
    uint16_t unalloc_blocks;
    uint16_t unalloc_inodex;
    uint16_t directories_in_group;
    
    uint8_t unused[14];
} __attribute__((packed));

struct ext2_superblock
{
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t blocks_res_for_superuser;
    uint32_t total_unallocated_blocks;
    uint32_t total_unallocated_inodes;
    uint32_t superblock_block_number; // number of the block that contains this structure
    uint32_t block_size_shl;          // the number to shift 1024 to the left to obtain block size
    uint32_t fragment_size_shl;       // the number to shift 1024 to the left to obtain fragment size
    uint32_t total_blocks_bg;         // number of blocks in block group
    uint32_t total_fragments_bg;      // number of fragments in block group
    uint32_t total_inodes_bg;         // number of inodes in block group
    uint32_t last_mount_time;         // posix time
    uint32_t last_write_time;         // posix time
    uint16_t mounts_since_consistency_check;
    uint16_t mounts_before_consistency_check;
    uint16_t signature;
    uint16_t fs_state; // see EXT2_FS_STATES
    uint16_t on_error; // see EXT2_FS_ERRORS
    uint16_t minor_version;
    uint32_t last_consistency_time;               // posix time
    uint32_t interval_between_forced_consistency; // posix time
    uint32_t os_id;
    uint32_t major_version;
    uint16_t user_id;  // user ID that can use reserved blocks
    uint16_t group_id; // group ID that can use reserved blocks

    // can be used only with if EXT2_EXTENDED() is true
    struct
    {
        uint32_t first_inode;  // if EXT2_EXTENDED() == false => fixed to 11
        uint16_t sizeof_inode; // if EXT2_EXTENDED() == false => fixed to 128
        uint16_t block_group;
        uint32_t optional_features;
        uint32_t required_features;
        uint32_t unsupported_features;
        uint8_t fs_id[16];       // "blkid" outputs this
        uint8_t volume_name[16]; // C-style NULL-terminated string
        uint8_t last_mount_path[64];
        uint32_t compression_algorithm; // if any used
        uint8_t number_of_prealloc_blocks_files;
        uint8_t number_of_prealloc_blocks;
        uint16_t unused0;
        uint8_t journal_id[16];
        uint32_t journal_inode;
        uint32_t journal_device;
        uint32_t head_of_orphan_inode;
        uint8_t unused1[788];
    } extended;

} __attribute__((packed));

struct ext2_info
{
    // the target disk
    struct driver_disk disk_device;

    struct ext2_superblock superblock;

    struct ext2_block *block_groups;
    uint32_t block_groups_count;
    
    uint8_t read_only;
};

enum EXT2_STATUS
{
    EXT2_SUCCESS = 0,
    EXT2_INVALID = 1,

    EXT2_EMPTY = 2,
};

int ext2_load(struct ext2_info *ext, struct driver_disk disk);
void ext2_cleanup(struct ext2_info *ext);
