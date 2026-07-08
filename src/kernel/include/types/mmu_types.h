#ifndef __MMU_TYPES_H__
#define __MMU_TYPES_H__

#include "macros.h"

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

typedef union {
    u64 value;
    struct {
        u64 valid        : 1;  // [0]
        u64 type         : 1;  // [1], 1 = page
        u64 attr_index   : 3;  // [4:2]
        u64 ns           : 1;  // [5]
        u64 ap           : 2;  // [7:6]
        u64 sh           : 2;  // [9:8]
        u64 af           : 1;  // [10]
        u64 ng           : 1;  // [11]
        u64 address      : 36; // bits [47:12]
        u64 reserved     : 3;  // [50:48]
        u64 dbm          : 1;  // [51], dirty bit modifier
        u64 contiguous   : 1;  // [52]
        u64 pxn          : 1;  // [53]
        u64 uxn          : 1;  // [54]
        u64 ignored      : 4;  // [58:55]
        u64 software     : 5;  // [63:59]
    } __attribute__((packed)) bits;
} mem_descriptor_t;

typedef union {
    u64 value;
    struct {
        u64 valid        : 1;   // bit 0
        u64 type         : 1;   // bit 1 (1 = table, 0 = block)
        u64 reserved1    : 10;  // bits 2-11
        u64 address      : 36;  // bits 12-47
        u64 reserved2    : 4;   // bits 48-51
        u64 pxn          : 1;   // bit 52
        u64 uxn          : 1;   // bit 53
        u64 ignored      : 9;   // bits 54-62
        u64 reserved3    : 1;   // bit 63, to make 64 bits total
    } __attribute__((packed)) bits;
} table_descriptor_t;


typedef union {
    u64 value;

    /* Page or Block descriptor */
    struct {
        u64 valid        : 1;  // [0]
        u64 type         : 1;  // [1], 1 = page
        u64 attr_index   : 3;  // [4:2]
        u64 ns           : 1;  // [5]
        u64 ap           : 2;  // [7:6]
        u64 sh           : 2;  // [9:8]
        u64 af           : 1;  // [10]
        u64 ng           : 1;  // [11]
        u64 address      : 36; // bits [47:12]
        u64 reserved     : 3;  // [50:48]
        u64 dbm          : 1;  // [51], dirty bit modifier
        u64 contiguous   : 1;  // [52]
        u64 pxn          : 1;  // [53]
        u64 uxn          : 1;  // [54]
        u64 cow          : 1;  // [55]
        u64 ignored      : 3;  // [58:56]
        u64 software     : 5;  // [63:59]
    } __attribute__((packed)) md;

    /* Table descriptor */
    struct {
        u64 valid        : 1;   // bit 0
        u64 type         : 1;   // bit 1 (1 = table, 0 = block)
        u64 reserved1    : 10;  // bits 2-11
        u64 address      : 36;  // bits 12-47
        u64 reserved2    : 4;   // bits 48-51
        u64 pxn          : 1;   // bit 52
        u64 uxn          : 1;   // bit 53
        u64 ignored      : 9;   // bits 54-62
        u64 reserved3    : 1;   // bit 63, to make 64 bits total
    } __attribute__((packed)) td;
} pte;

#endif
