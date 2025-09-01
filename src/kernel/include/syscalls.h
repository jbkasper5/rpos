#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include "macros.h"

#define SYS_MAX                     292


#define SYS_IO_SETUP                0
#define SYS_IO_DESTROY              1
#define SYS_IO_SUBMIT               2
#define SYS_IO_CANCEL               3
#define SYS_IO_GETEVENTS            4
#define SYS_SETXATTR                5
#define SYS_LSETXATTR               6

#define SYS_GETCWD                  17
#define SYS_DUP                     23
#define SYS_DUP3                    24
#define SYS_READ                    63
#define SYS_WRITE                   64
#define SYS_EXIT                    93
#define SYS_KILL                    129
#define SYS_REBOOT                  142
#define SYS_GETPID                  172



typedef uint64_t (*syscall_fn_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3);
uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3);

syscall_fn_t syscall_table[SYS_MAX] = {
    [SYS_WRITE] = sys_write,
    [SYS_READ]  = sys_read,
};

#endif