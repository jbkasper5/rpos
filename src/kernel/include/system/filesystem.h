#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "macros.h"

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
} directory_entry;

void print_partitions(mbr* mbr);
void print_directory(sector* s);

#endif