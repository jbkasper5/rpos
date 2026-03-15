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
} fdt_header;

typedef struct {
    uint32_t len;
    uint32_t nameoff;
} fdt_property;

void device_tree_init();

#endif