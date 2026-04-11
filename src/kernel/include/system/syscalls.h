#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include "uapi/rpos/syscall_macros.h"
#include "uapi/rpos/clone.h"
#include "macros.h"

#define SYS_ERROR           -1ULL
#define SYS_SUCCESS         0

typedef u64 (*syscall_fn_t)(u64, u64, u64, u64, u64, u64);


u64 sys_ioctl(u64 fd, u64 cmd, u64 arg, u64 unused1, u64 unused2, u64 unused3);
u64 sys_io_setup(u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5, u64 unused6);
u64 sys_write(u64 fd, u64 buf, u64 count, u64 unused1, u64 unused2, u64 unused3);
u64 sys_read(u64 fd, u64 buf, u64 count, u64 unused1, u64 unused2, u64 unused3);
u64 sys_nanosleep(u64 ns, u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5);
u64 sys_clock_gettime(u64 clock, u64 kernel_timespec, u64 unused2, u64 unused3, u64 unused4, u64 unused5);
u64 sys_pulse_led(u64 pin_num, u64 turn_on, u64 unused1, u64 unused2, u64 unused3, u64 unused4);
u64 sys_mmap(u64 addr, u64 len, u64 prot, u64 flags, u64 fd, u64 offset);
u64 sys_munmap(u64 addr, u64 len, u64 unused1, u64 unused2, u64 unused3, u64 unused4);
u64 sys_clone3(u64 cl_args, u64 size, u64 unused1, u64 unused2, u64 unused3, u64 unused4);
u64 sys_execve(u64 path, u64 argv, u64 envp, u64 unused1, u64 unused2, u64 unused3);
u64 sys_exit_group(u64 status, u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5);
u64 sys_get_framebuffer(u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5, u64 unused6);
u64 sys_open(u64 path, u64 flags, u64 unused3, u64 unused4, u64 unused5, u64 unused6);
u64 sys_getc(u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5, u64 unused6);
u64 sys_pipe2(u64 fd_rets, u64 flags, u64 unused1, u64 unused2, u64 unused3, u64 unused4);
u64 sys_fork(u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5, u64 unused6);

syscall_fn_t syscall_table[SYS_MAX] = {
    [SYS_IOCTL] = sys_ioctl,
    [SYS_IO_SETUP] = sys_io_setup,
    [SYS_OPEN] = sys_open,
    [SYS_WRITE] = sys_write,
    [SYS_READ]  = sys_read,
    [SYS_MMAP] = sys_mmap,
    [SYS_CLONE3] = sys_clone3,
    [SYS_EXECVE] = sys_execve,
    [SYS_MUNMAP] = sys_munmap,
    [SYS_EXIT_GROUP] = sys_exit_group,
    [SYS_NANOSLEEP] = sys_nanosleep,
    [SYS_CLOCK_GETTIME] = sys_clock_gettime,
    [SYS_PULSE_LED] = sys_pulse_led,
    [SYS_GET_FRAMEBUFFER] = sys_get_framebuffer,
    [SYS_GETC] = sys_getc,
    [SYS_PIPE2] = sys_pipe2,
    [SYS_FORK] = sys_fork,
};

#endif