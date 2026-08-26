#ifndef __DISK_H__
#define __DISK_H__

#include "macros.h"
#include "types/disk_types.h"

#define USER_FILESYSTEM_BASE 0x104000

extern fs rootfs;


void filesystem_init();
void print_partitions(mbr* mbr);
void print_fat32_directory(sector* s);
void read_block(void* buf, u32 block_num);

ext4_inode* lookup(ext4_inode* dirnode, char* name);

#endif
