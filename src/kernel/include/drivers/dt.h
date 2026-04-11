#ifndef __DT_H__
#define __DT_H__

#include "macros.h"
#include "asm_utils.h"
#include "utils/utils.h"

typedef enum {
    FDT_BEGIN_NODE = 0x1,
    FDT_END_NODE = 0x2,
    FDT_PROP = 0x3,
    FDT_NOP = 0x4,
    FDT_END = 0x9,
} ftd_tokens;

typedef struct {
    u32 magic;             /* 0xd00dfeed */
    u32 totalsize;         /* Total size of the blob */
    u32 off_dt_struct;     /* Offset to structure block */
    u32 off_dt_strings;    /* Offset to strings block */
    u32 off_mem_rsvmap;    /* Offset to reservation block */
    u32 version;           /* Format version */
    u32 last_comp_version; /* Compatible version */
    u32 boot_cpuid_phys;   /* Physical CPU ID */
    u32 size_dt_strings;   /* Size of strings block */
    u32 size_dt_struct;    /* Size of structure block */
} fdt_header;

typedef struct {
    u32 len;
    u32 nameoff;
} fdt_property;

void device_tree_init();

#endif