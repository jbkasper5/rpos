#ifndef __DT_H__
#define __DT_H__

#include "macros.h"
#include "asm_utils.h"

typedef struct {
    uint32_t magic;             /* 0xd00dfeed */
    uint32_t totalsize;         /* Total size of the blob */
    uint32_t off_dt_struct;     /* Offset to structure block */
    uint32_t off_dt_strings;    /* Offset to strings block */
    uint32_t off_mem_rsvmap;    /* Offset to reservation block */
    uint32_t version;           /* Format version */
    uint32_t last_comp_version; /* Compatible version */
    uint32_t boot_cpuid_phys;   /* Physical CPU ID */
    uint32_t size_dt_strings;   /* Size of strings block */
    uint32_t size_dt_struct;    /* Size of structure block */
} dt_header;

void device_tree_init();

#endif