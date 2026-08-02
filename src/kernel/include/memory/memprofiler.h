#ifndef __MEMPROFILER_H__

#include "types/scalar.h"
#include "io/kprintf.h"


typedef struct {
    // WHAT to scan
    u64 start_pfn;    // default 0
    u64 count;        // 0 => whole covered PFA
    u32 stride;       // pages per glyph; 0/1 => one page each

    // HOW to render
    // profile_lens lens;   // default LENS_STATE

    // FILTER (optional)
    u8   state_mask;  // 0 => all; else (1<<PAGE_BUDDY)|... to restrict
    bool only_shared; // refcount > 1 only
    u32  pid;         // 0 => all; else only this owner  (see caveat)

    // LAYOUT
    u32  per_line;    // 0 => 64
    bool legend;
    bool summary;     // totals at the end
} memprofiler_cfg;


void profile(memprofiler_cfg* cfg);
void ram_profile();

#define __MEMPROFILER_H__
#endif