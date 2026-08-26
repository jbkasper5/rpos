/*
 * Host environment for exercising kernel memory code off-target.
 *
 * Model: a physical address is simply a byte offset into one big lazily-backed
 * host buffer (`host_ram()`). The kernel's pa_to_va / va_to_pa become
 * `ram +/- offset`, so all of the allocator's pointer arithmetic and, later,
 * the page-table walker's dereferences land inside real host memory.
 *
 * This header is the only thing test files need beyond the kernel's own
 * paging.h: it exposes the RAM handle, a direct pointer to the frame_metadata
 * array for assertions, and the reserved-prefix length the stubs model.
 */
#ifndef HOST_ENV_H
#define HOST_ENV_H

#include "memory/mmap.h"   /* -> paging_types.h (page_frame_t), scalar.h (u64,bool), MAP_* */
#include "asm_utils.h"      /* page_frame_array_start/end, get_phys_test_region, ... */

#define HOST_RAM_BYTES  (1ULL << 30)   /* 1 GiB of simulated physical RAM        */
#define HOST_NPAGES     (1ULL << 18)   /* pages the PFA covers (matches kernel)   */

void            host_env_init(void);        /* mmap the RAM region (idempotent)   */
unsigned char  *host_ram(void);             /* base of simulated physical RAM     */
page_frame_t   *host_pfa(void);             /* == kernel frame_metadata           */
u64             host_reserved_pages(void);  /* reserved-prefix length, in pages   */

/* non-static kernel symbol not declared in paging.h */
extern void buddy_free(void *page);

#endif /* HOST_ENV_H */
