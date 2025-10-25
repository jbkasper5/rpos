#ifndef __SYSCALL_MACROS_H__
#define __SYSCALL_MACROS_H__

#include "macros.h"

#define SYS_MAX                     300


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
#define SYS_NANOSLEEP               101
#define SYS_KILL                    129
#define SYS_REBOOT                  142
#define SYS_GETPID                  172

#define SYS_MUNMAP                  215
#define SYS_EXECVE                  221
#define SYS_MMAP                    222

#define SYS_PULSE_LED               292

#endif