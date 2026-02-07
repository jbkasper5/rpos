#ifndef __DISK_H__
#define __DISK_H__

#include "macros.h"

#define USER_FILESYSTEM_BASE 0x104000

typedef struct PACKED {
    uint8_t head;
    uint8_t sector : 6;
    uint8_t cylinder_hi : 2;
    uint8_t cylinder_lo;
} chs_address ;

typedef struct PACKED {
    uint8_t status;
    chs_address first_sector;
    uint8_t type;
    chs_address last_sector;
    uint32_t first_lba_sector;
    uint32_t num_sectors;
} partition_entry;

typedef struct PACKED {
    uint8_t boot_code[0x1BE];
    partition_entry partitions[4];
    uint16_t boot_signature;
} mbr;

typedef struct {
    uint8_t data[512];
} sector;

typedef struct {
    char filename[8];
    char extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t created_at_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_accessed;
    uint16_t high_word;
    uint16_t last_modified_time;
    uint16_t last_modified_date;
    uint16_t low_word;
    uint32_t file_size;
} fat32_directory_entry;

typedef struct PACKED {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    int32_t  s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    int16_t  s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    /* EXT4_DYNAMIC_REV specific */
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    uint32_t s_algo_bitmap;
    uint8_t  s_prealloc_blocks;
    uint8_t  s_prealloc_dir_blocks;
    uint16_t s_reserved_gdt_blocks;
    uint8_t  s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t s_jnl_backup_type;
    uint8_t s_desc_size;
    /* ... rest can be ignored for now */
} ext_superblock;

typedef struct PACKED {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size_lo;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks_lo;
    uint32_t i_flags;       // 0x80000 -> means extent tree
    uint32_t i_osd1;
    uint32_t i_block[15];           // this is a pointer to the extent tree
    uint32_t i_generation;
    uint32_t i_file_acl_lo;
    uint32_t i_size_high;
    uint32_t i_obso_faddr;
    uint32_t i_osd2[3];

    // in docs, but not meant for 128-byte inodess
    // uint16_t i_extra_isize;
    // uint16_t i_checksum_hi;
    // uint32_t i_ctime_extra;
    // uint32_t i_mtime_extra;
    // uint32_t i_atime_extra;
    // uint32_t i_crtime;
    // uint32_t i_crtime_extra;
    // uint32_t i_version_hi;
    // uint32_t i_projid;
} ext4_inode;

typedef struct PACKED {
    uint16_t eh_magic;      // 0xF30A
    uint16_t eh_entries;    // number of valid entries
    uint16_t eh_max;        // max entries possible
    uint16_t eh_depth;      // 0 = leaf, >0 = internal node (at most 5)
    uint32_t eh_generation;
} ext4_extent_header;

typedef struct PACKED {
    uint32_t ee_block;      // first file block
    uint16_t ee_len;        // number of blocks
    uint16_t ee_start_hi;
    uint32_t ee_start_lo;
} ext4_extent;

typedef struct PACKED {
    uint32_t ei_block;
    uint32_t ei_leaf_lo;
    uint16_t ei_leaf_hi;
    uint32_t ei_unused;
} ext4_extent_idx;

typedef struct PACKED {
    uint32_t inode;       // inode number of the file/directory
    uint16_t rec_len;     // length of this directory entry
    uint8_t  name_len;    // length of the name
    uint8_t  file_type;   // type of file (optional in ext4)
    char     name[];      // the filename, not null-terminated
} ext4_dir_entry;

typedef struct {
    sector data[8];
} ext4_block;

typedef struct PACKED {
    uint32_t bg_block_bitmap_lo;     // start of block bitmap
    uint32_t bg_inode_bitmap_lo;     // start of inode bitmap
    uint32_t bg_inode_table_lo;      // start of inode table (the one you want)
    uint16_t bg_free_blocks_count_lo;
    uint16_t bg_free_inodes_count_lo;
    uint16_t bg_used_dirs_count_lo;
    uint16_t bg_flags;
    uint32_t bg_exclude_bitmap_lo;
    uint16_t bg_block_bitmap_csum_lol;
    uint16_t bg_inode_bitmap_csum_lo;
    uint16_t bg_itable_unused_lo;
    uint16_t bg_checksum;
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
void read_block(void* buf, uint32_t block_num);

ext4_inode* lookup(ext4_inode* dirnode, char* name);

#endif