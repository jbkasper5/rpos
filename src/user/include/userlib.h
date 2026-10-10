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

typedef volatile u32 reg32_t;
typedef volatile u64 reg64_t;
typedef u8 bool;
typedef u16 pid_t;



/*
 * Raw syscall trampoline. Defined per-project in syscall.S:
 *   x0 = syscall number, x1..x6 = arguments.
 */
extern u64 syscall(u64 sys_num, ...);

/*
 * Buffered, ANSI-aware printf writing to fd 1 via SYS_WRITE.
 * Supported conversions: %d %l %s %c %x %f %%
 * Output is flushed on newline and when the 2 KiB internal buffer fills.
 */
void printf(char* format_str, ...);

#endif
