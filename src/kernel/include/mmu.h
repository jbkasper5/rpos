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


void mmu_init();
void initialize_page_tables(void* ptb, pt_metadata_t* pt_metadata);
void create_kernel_identity_mapping(pt_metadata_t* pt0_metadata);
void create_peripheral_identity_mapping(pt_metadata_t* l0_page_table_metadata);
void create_user_mapping(void* l0_page_table);
void _alloc_pt_metadata(pt_level_t level);


/*
// first L2 2MiB block entry starting at address 0x0: 0x705 = 0b 0111 0000 0101
// next L2 2MiB block entry starting at address 0x40000000: 0x40000705

// 0x705 -> 01111 0000 0101
// --> 0 1 1 11 00 0 001 0 1
// --> addr = 0, ng = 1, af = 1, sh = 11, ap = 00, ns = 0, attr_idx = 001, type = 0, valid = 1
*/



/*

// peripheral base - 32 bit address
0xFE000000

// in 64 bit, 
0x00000000FE000000

top 16 bits ignored as virtual address:
0x0000FE000000

As binary:
   0    0    0    0    F    E    0    0    0    0    0    0
0b0000 0000 0000 0000 1111 1110 0000 0000 0000 0000 0000 0000

top 9 bits as L0 page table index:
0000 0000 0

next 9 bits as L2 page table index
000 0000 11     (index 3)

next 9 bits as L3 page table index (or block offset)
11 1110 000

next 9 bits as l4 page table index 
0 0000 0000

last 12 bits as page offset
0000 0000 0000 0000
*/


#endif



/*



0xF41

-> 0b111101000001

0b1111010000    01
valid, block descriptor

0b1111010   000    01
attr_idx = 0

0b111101    0   000    01
ns = 0

0b1111  01    0   000    01
ap = 01

0b11    11  01    0   000    01
sh = 11

0b1 1    11  01    0   000    01
af = 1

0b1 1    11  01    0   000    01
ng = 1
*/