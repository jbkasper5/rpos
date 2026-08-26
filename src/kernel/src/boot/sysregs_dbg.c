#include "macros.h"

typedef struct {
    u64 sctlr, tcr, mair, ttbr0, ttbr1, cpuectlr;
} sysreg_snapshot_t;

void snapshot_sysregs(void) {
    volatile sysreg_snapshot_t g_sysregs;
    
    asm volatile("mrs %0, sctlr_el1"     : "=r"(g_sysregs.sctlr));
    asm volatile("mrs %0, tcr_el1"       : "=r"(g_sysregs.tcr));
    asm volatile("mrs %0, mair_el1"      : "=r"(g_sysregs.mair));
    asm volatile("mrs %0, ttbr0_el1"     : "=r"(g_sysregs.ttbr0));
    asm volatile("mrs %0, ttbr1_el1"     : "=r"(g_sysregs.ttbr1));
    asm volatile("mrs %0, S3_1_c15_c2_1" : "=r"(g_sysregs.cpuectlr));
}