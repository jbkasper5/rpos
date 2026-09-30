#ifndef __USERLIB_H__
#define __USERLIB_H__

#include <stdarg.h>
#include "uabi/rpos/syscall_macros.h"


/*
 * Raw syscall trampoline. Defined per-project in syscall.S:
 *   x0 = syscall number, x1..x6 = arguments.
 */
extern int syscall(unsigned long sys_num, ...);

/*
 * Buffered, ANSI-aware printf writing to fd 1 via SYS_WRITE.
 * Supported conversions: %d %l %s %c %x %f %%
 * Output is flushed on newline and when the 2 KiB internal buffer fills.
 */
void printf(char* format_str, ...);

#endif
