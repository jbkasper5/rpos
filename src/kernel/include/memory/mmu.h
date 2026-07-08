#ifndef __MMU_H__
#define __MMU_H__

#include "macros.h"
#include "memory/mem.h"
#include "utils/utils.h"
#include "types/mmu_types.h"

extern u64* L0_TABLE;

#define NO_EXECUTE      1
#define ALLOW_EXECUTE   008

#define INVALID_PT_METADATA             -1

// 4 GiB RAM = 2 ^ 32
// 4 KiB pages = 2 ^ 12
// 2 ^ 20 = (1 << 20) pages = (1 << 20) bytes given 1B metadata per page

void mmu_init();
u64* finish_virtual_mapping();
u64* initialize_page_tables();
u64* create_kernel_identity_mapping();

#endif
