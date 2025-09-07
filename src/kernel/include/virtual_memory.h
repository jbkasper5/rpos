#ifndef __VIRTUAL_MEMORY_H__
#define __VIRTUAL_MEMORY_H__

#define PAGE_OFFSET_BITS                12
#define PAGE_SIZE                       (1 << PAGE_OFFSET_BITS)
#define PAGE_TABLE_BITS                 9
#define PAGE_TABLE_SIZE                 (1 << PAGE_TABLE_BITS)
#define PTE_SIZE                        64      // 64 bits

#define TCR_EL1_VALUE                   ((1 << 4) | (1 << 7))


#endif