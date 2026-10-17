#ifndef __USERLIB_H__
#define __USERLIB_H__

#include <stdarg.h>
#include <stdint.h>
#include "uabi/rpos/syscall_macros.h"
#include "uabi/rpos/mman.h"


typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;



/*
 * Raw syscall trampoline. Defined per-project in syscall.S:
 *   x0 = syscall number, x1..x6 = arguments.
 */
extern u64 syscall(u64 sys_num, ...);

/*
 * printf. Which one depends on the build flavor (set by src/user/Makefile):
 *
 *   musl         real libc stdio. RPOS_MUSL is defined, so <stdio.h> supplies
 *                int printf(const char* restrict, ...).
 *   freestanding our own buffered, ANSI-aware printf from lib/printf.c,
 *                writing to fd 1 via SYS_WRITE.
 *                Supported conversions: %d %l %s %c %x %f %%
 *                Flushed on newline and when the 2 KiB buffer fills.
 *
 * The guard is required, not cosmetic: the bare-metal toolchain ships newlib
 * headers even under -ffreestanding, so an unconditional <stdio.h> declares
 * `int printf(const char* restrict, ...)` and lib/printf.c's own definition
 * fails to compile with "conflicting types for 'printf'".
 */
#ifdef RPOS_MUSL
#include <stdio.h>
#else
void printf(char* format_str, ...);
#endif

#endif
