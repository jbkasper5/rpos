/*
 * The one harness TU that talks to libc. Includes system headers and NO kernel
 * headers, so the host's pid_t is fine here and never meets the kernel's.
 *
 * Besides the allocator, this file DEFINES two kernel-referenced symbols
 * (panic, kprintf) that need libc. Their kernel prototypes live in kernel
 * headers we can't include here without a pid_t clash, but the definitions are
 * ABI-compatible and the linker matches them by name.
 */
#include "host_sys.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>

/* framework globals (defined in test.c) */
extern jmp_buf g_test_jmp;
extern int     g_test_failed;

void *host_sys_alloc(unsigned long bytes) {
    /* page-aligned; large allocations are demand-zero on macOS/Linux, so this
     * costs only the pages the tests actually touch (~the 8 MiB PFA + a few). */
    void *p = aligned_alloc(4096, bytes);
    if (!p) {
        fprintf(stderr, "aligned_alloc of simulated RAM failed\n");
        exit(2);
    }
    return p;
}

/* --- kernel-referenced symbols that need libc --- */

void panic(void) {
    fprintf(stderr, "    !!! kernel panic() reached\n");
    g_test_failed = 1;
    longjmp(g_test_jmp, 1);
}

void kprintf(char *fmt, ...) {
    /* silent by default so test output stays clean; HOST_KLOG=1 to see it */
    if (getenv("HOST_KLOG")) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
}
