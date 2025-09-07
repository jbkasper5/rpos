#ifndef __MMU_H__
#define __MMU_H__

#include "macros.h"
#include "mem.h"
#include "utils.h"

typedef union {
    uint64_t value;
    struct {
        uint64_t valid        : 1;
        uint64_t type         : 1;  // 1 = page
        uint64_t attr_index   : 3;  // bits [2,8:9]
        uint64_t ns           : 1;
        uint64_t ap           : 2;
        uint64_t sh           : 2;
        uint64_t af           : 1;
        uint64_t ng           : 1;
        uint64_t address      : 36; // bits [47:12]
        uint64_t reserved     : 4;
        uint64_t contiguous   : 1;
        uint64_t pxn          : 1;
        uint64_t uxn          : 1;
        uint64_t ignored      : 4;
        uint64_t software     : 4;
    } __attribute__((packed)) bits;
} page_descriptor_t;

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


void mmu_init();
void initialize_page_tables(void* page_table_base);

#endif