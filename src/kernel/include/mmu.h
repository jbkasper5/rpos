#ifndef __MMU_H__
#define __MMU_H__

#include "macros.h"
#include "mem.h"
#include "utils.h"

typedef enum {
    EL0_NA_EL1_RW = 0,
    EL0_RW_EL1_RW = 1,
    EL0_NA_EL1_RO = 2,
    EL0_RO_EL1_RO = 3
} AccessPermissions_t;

typedef enum {
    L1_BLOCK = 1,
    L2_BLOCK = 2,
    PAGE = 3
} blocktype_t;

#define NO_EXECUTE      1
#define ALLOW_EXECUTE   008

typedef union {
    uint64_t value;
    struct {
        uint64_t valid        : 1;  // [0]
        uint64_t type         : 1;  // [1], 1 = page
        uint64_t attr_index   : 3;  // [4:2]
        uint64_t ns           : 1;  // [5]
        uint64_t ap           : 2;  // [7:6]
        uint64_t sh           : 2;  // [9:8]
        uint64_t af           : 1;  // [10]
        uint64_t ng           : 1;  // [11]
        uint64_t address      : 36; // bits [47:12]
        uint64_t reserved     : 3;  // [50:48]
        uint64_t dbm          : 1;  // [51], dirty bit modifier
        uint64_t contiguous   : 1;  // [52]
        uint64_t pxn          : 1;  // [53]
        uint64_t uxn          : 1;  // [54]
        uint64_t ignored      : 4;  // [58:55]
        uint64_t software     : 5;  // [63:59]
    } __attribute__((packed)) bits;
} mem_descriptor_t;

typedef union {
    uint64_t value;
    struct {
        uint64_t valid        : 1;   // bit 0
        uint64_t type         : 1;   // bit 1 (1 = table, 0 = block)
        uint64_t reserved1    : 10;  // bits 2-11
        uint64_t address      : 36;  // bits 12-47
        uint64_t reserved2    : 4;   // bits 48-51
        uint64_t pxn          : 1;   // bit 52
        uint64_t uxn          : 1;   // bit 53
        uint64_t ignored      : 9;   // bits 54-62
        uint64_t reserved3    : 1;   // bit 63, to make 64 bits total
    } __attribute__((packed)) bits;
} table_descriptor_t;

#define INVALID_PT_METADATA             -1

typedef enum pt_level_s{
    PT_LEVEL0 = 0,
    PT_LEVEL1 = 1,
    PT_LEVEL2 = 2,
    PT_LEVEL3 = 3
} pt_level_t;

typedef struct pt_metadata_s{
    void* table_address;            // physical address of the page table being described
    struct pt_metadata_s* next;     // next metadata entry
    pt_level_t level;               // which level page table this metadata is describing
    uint16_t count;                 // how many entries in the table are in use
    int16_t index;                  // signed int describing which entry in the parent table this belongs, -1 means invalid
} pt_metadata_t;

typedef union {
    uint8_t value;
    struct {
        uint8_t owner         : 1;
        uint8_t allocated     : 1;
        uint8_t reserved      : 6;
    } __attribute__((packed)) bits;
} page_frame_t;

// 4 GiB RAM = 2 ^ 32
// 4 KiB pages = 2 ^ 12
// 2 ^ 20 = (1 << 20) pages = (1 << 20) bytes given 1B metadata per page

void mmu_init();
void initialize_page_tables(void* ptb, pt_metadata_t* pt_metadata);
void create_kernel_identity_mapping();
void create_peripheral_identity_mapping(pt_metadata_t* l0_page_table_metadata);
void create_user_mapping(pt_metadata_t* pt0_metadata);
void _alloc_pt_metadata(pt_level_t level);
#endif