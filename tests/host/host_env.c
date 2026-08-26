/*
 * Host stubs for the symbols the kernel memory TUs reference but do not define.
 * Nothing here modifies the kernel; it only satisfies the linker and models
 * "RAM" as a flat host buffer.
 *
 *   physical address  <->  byte offset into g_ram
 *   pa_to_va(pa) = g_ram + pa      va_to_pa(va) = va - g_ram
 *
 * This TU includes ONLY kernel headers (never libc) -- see the pid_t note in
 * test.h. Anything needing libc goes through host_sys.h.
 *
 * Reserved layout (mirrors the on-target prefix so the buddy allocator never
 * hands back memory that is "in use"):
 *
 *   [0 .. PFA_PHYS)              low RAM (reserved)
 *   [PFA_PHYS .. +8 MiB)         frame_metadata array itself (reserved)
 *   [.. TEST_PHYS+TEST_BYTES)    "test section" (reserved)
 *   [reserved_end .. 1 GiB)      buddy-managed free RAM
 */
#include "host_env.h"        /* -> memory/mmap.h: page_frame_t, u64, bool, MAP_* */
#include "asm_utils.h"
#include "utils/utils.h"
#include "host_sys.h"

#define PFA_PHYS    0x100000ULL   /* frame_metadata base:            1 MiB */
#define TEST_PHYS   0xA00000ULL   /* reserved "test section" base:  10 MiB */
#define TEST_BYTES  0x2000ULL     /* 2 pages                               */

static unsigned char *g_ram = NULL;

static void ensure_ram(void) {
    if (!g_ram) g_ram = host_sys_alloc(HOST_RAM_BYTES);
}

void            host_env_init(void)        { ensure_ram(); }
unsigned char  *host_ram(void)             { ensure_ram(); return g_ram; }
page_frame_t   *host_pfa(void)             { return (page_frame_t *)(uintptr_t)page_frame_array_start(); }
u64             host_reserved_pages(void)  { return ((TEST_PHYS + TEST_BYTES) + 0xFFF) >> 12; }

/* ---- address model ---- */
u64 pa_to_va(u64 pa) { ensure_ram(); return (u64)(uintptr_t)(g_ram + pa); }
u64 va_to_pa(u64 va) { ensure_ram(); return (u64)((unsigned char *)(uintptr_t)va - g_ram); }

/* ---- linker / asm accessors ---- */
u64 page_frame_array_start(void) { ensure_ram(); return (u64)(uintptr_t)(g_ram + PFA_PHYS); }
u64 page_frame_array_end(void)   { ensure_ram(); return (u64)(uintptr_t)(g_ram + PFA_PHYS + HOST_NPAGES * sizeof(page_frame_t)); }
u64 get_phys_test_region(void)   { return TEST_PHYS; }
u64 get_test_size(void)          { return TEST_BYTES; }
u64 virt_base(void)              { ensure_ram(); return (u64)(uintptr_t)g_ram; }

double seconds_since_boot(void)  { return 0.0; }

/* ---- link-satisfying stubs (not exercised by the buddy/PFA cut) ---- */
u64 *L0_TABLE = NULL;

bool map(u64 v, u64 p, u8 o, u64 f, u64 b) {
    (void)v; (void)p; (void)o; (void)f; (void)b;
    return 1;
}

void flush_tlb(void) {}

/* utils.S provides this on-target; floor(log2) is exact for kmalloc's
 * already-power-of-2 inputs */
int log2_pow2(u64 n) {
    int r = 0;
    while (n >>= 1) r++;
    return r;
}
