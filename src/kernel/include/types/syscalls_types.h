#ifndef __SYSCALLS_TYPES_H__
#define __SYSCALLS_TYPES_H__

#include "macros.h"

typedef u64 (*syscall_fn_t)(u64, u64, u64, u64, u64, u64);

#endif
