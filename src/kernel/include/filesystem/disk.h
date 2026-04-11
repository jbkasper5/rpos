#ifndef __DISK_H__
#define __DISK_H__

#include "macros.h"

#define USER_FILESYSTEM_BASE 0x104000

typedef struct PACKED {
    u8 head;
    u8 sector : 6;
    u8 cylinder_hi : 2;
    u8 cylinder_lo;
} chs_address ;

typedef struct PACKED {
    u8 status;
    chs_address first_sector;
    u8 type;
    chs_address last_sector;
    u32 first_lba_sector;
    u32 num_sectors;
} partition_entry;

typedef struct PACKED {
    u8 boot_code[0x1BE];
    partition_entry partitions[4];
    u16 boot_signature;
} mbr;

typedef struct {
    u8 data[512];
} sector;

typedef struct {
    char filename[8];
    char extension[3];
    u8 attributes;
    u8 reserved;
    u8 created_at_tenths;
    u16 creation_time;
    u16 creation_date;
    u16 last_accessed;
    u16 high_word;
    u16 last_modified_time;
    u16 last_modified_date;
    u16 low_word;
    u32 file_size;
} fat32_directory_entry;

typedef struct PACKED {
    u32 s_inodes_count;
    u32 s_blocks_count;
    u32 s_r_blocks_count;
    u32 s_free_blocks_count;
    u32 s_free_inodes_count;
    u32 s_first_data_block;
    u32 s_log_block_size;
    int32_t  s_log_frag_size;
    u32 s_blocks_per_group;
    u32 s_frags_per_group;
    u32 s_inodes_per_group;
    u32 s_mtime;
    u32 s_wtime;
    u16 s_mnt_count;
    int16_t  s_max_mnt_count;
    u16 s_magic;
    u16 s_state;
    u16 s_errors;
    u16 s_minor_rev_level;
    u32 s_lastcheck;
    u32 s_checkinterval;
    u32 s_creator_os;
    u32 s_rev_level;
    u16 s_def_resuid;
    u16 s_def_resgid;
    /* EXT4_DYNAMIC_REV specific */
    u32 s_first_ino;
    u16 s_inode_size;
    u16 s_block_group_nr;
    u32 s_feature_compat;
    u32 s_feature_incompat;
    u32 s_feature_ro_compat;
    u8  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    u32 s_algo_bitmap;
    u8  s_prealloc_blocks;
    u8  s_prealloc_dir_blocks;
    u16 s_reserved_gdt_blocks;
    u8  s_journal_uuid[16];
    u32 s_journal_inum;
    u32 s_journal_dev;
    u32 s_last_orphan;
    u32 s_hash_seed[4];
    u8 s_def_hash_version;
    u8 s_jnl_backup_type;
    u8 s_desc_size;
    /* ... rest can be ignored for now */
} ext_superblock;

typedef struct PACKED {
    u16 i_mode;
    u16 i_uid;
    u32 i_size_lo;
    u32 i_atime;
    u32 i_ctime;
    u32 i_mtime;
    u32 i_dtime;
    u16 i_gid;
    u16 i_links_count;
    u32 i_blocks_lo;
    u32 i_flags;       // 0x80000 -> means extent tree
    u32 i_osd1;
    u32 i_block[15];           // this is a pointer to the extent tree
    u32 i_generation;
    u32 i_file_acl_lo;
    u32 i_size_high;
    u32 i_obso_faddr;
    u32 i_osd2[3];

    // in docs, but not meant for 128-byte inodess
    // u16 i_extra_isize;
    // u16 i_checksum_hi;
    // u32 i_ctime_extra;
    // u32 i_mtime_extra;
    // u32 i_atime_extra;
    // u32 i_crtime;
    // u32 i_crtime_extra;
    // u32 i_version_hi;
    // u32 i_projid;
} ext4_inode;

typedef struct PACKED {
    u16 eh_magic;      // 0xF30A
    u16 eh_entries;    // number of valid entries
    u16 eh_max;        // max entries possible
    u16 eh_depth;      // 0 = leaf, >0 = internal node (at most 5)
    u32 eh_generation;
} ext4_extent_header;

typedef struct PACKED {
    u32 ee_block;      // first file block
    u16 ee_len;        // number of blocks
    u16 ee_start_hi;
    u32 ee_start_lo;
} ext4_extent;

typedef struct PACKED {
    u32 ei_block;
    u32 ei_leaf_lo;
    u16 ei_leaf_hi;
    u32 ei_unused;
} ext4_extent_idx;

typedef struct PACKED {
    u32 inode;       // inode number of the file/directory
    u16 rec_len;     // length of this directory entry
    u8  name_len;    // length of the name
    u8  file_type;   // type of file (optional in ext4)
    char     name[];      // the filename, not null-terminated
} ext4_dir_entry;

typedef struct {
    sector data[8];
} ext4_block;

typedef struct PACKED {
    u32 bg_block_bitmap_lo;     // start of block bitmap
    u32 bg_inode_bitmap_lo;     // start of inode bitmap
    u32 bg_inode_table_lo;      // start of inode table (the one you want)
    u16 bg_free_blocks_count_lo;
    u16 bg_free_inodes_count_lo;
    u16 bg_used_dirs_count_lo;
    u16 bg_flags;
    u32 bg_exclude_bitmap_lo;
    u16 bg_block_bitmap_csum_lol;
    u16 bg_inode_bitmap_csum_lo;
    u16 bg_itable_unused_lo;
    u16 bg_checksum;
} ext4_group_desc_t;

typedef struct {
    ext_superblock* superblock;
    ext4_inode* root_inode;
    ext4_block* block_buf;
} fs;

extern fs rootfs;


void filesystem_init();
void print_partitions(mbr* mbr);
void print_fat32_directory(sector* s);
void read_block(void* buf, u32 block_num);

ext4_inode* lookup(ext4_inode* dirnode, char* name);

#endif