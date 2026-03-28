#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include "uapi/rpos/syscall_macros.h"
#include "macros.h"

#define SYS_ERROR           -1ULL
#define SYS_SUCCESS         0

typedef uint64_t (*syscall_fn_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

uint64_t sys_ioctl(uint64_t fd, uint64_t cmd, uint64_t arg, uint64_t unused1, uint64_t unused2, uint64_t unused3);

uint64_t sys_io_setup(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5, uint64_t unused6);

uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3);
uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3);
uint64_t sys_nanosleep(uint64_t ns, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);

uint64_t sys_clock_gettime(uint64_t clock, uint64_t kernel_timespec, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);

uint64_t sys_pulse_led(uint64_t pin_num, uint64_t turn_on, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);

uint64_t sys_mmap(uint64_t addr, uint64_t len, uint64_t prot, uint64_t flags, uint64_t fd, uint64_t offset);
uint64_t sys_munmap(uint64_t addr, uint64_t len, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);

uint64_t sys_execve(uint64_t path, uint64_t argv, uint64_t envp, uint64_t unused1, uint64_t unused2, uint64_t unused3);

uint64_t sys_exit_group(uint64_t status, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);

uint64_t sys_get_framebuffer(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5, uint64_t unused6);

uint64_t sys_open(uint64_t path, uint64_t flags, uint64_t unused3, uint64_t unused4, uint64_t unused5, uint64_t unused6);


uint64_t sys_getc(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5, uint64_t unused6);

syscall_fn_t syscall_table[SYS_MAX] = {
    [SYS_IOCTL] = sys_ioctl,
    [SYS_IO_SETUP] = sys_io_setup,
    [SYS_OPEN] = sys_open,
    [SYS_WRITE] = sys_write,
    [SYS_READ]  = sys_read,
    [SYS_MMAP] = sys_mmap,
    [SYS_EXECVE] = sys_execve,
    [SYS_MUNMAP] = sys_munmap,
    [SYS_EXIT_GROUP] = sys_exit_group,
    [SYS_NANOSLEEP] = sys_nanosleep,
    [SYS_CLOCK_GETTIME] = sys_clock_gettime,
    [SYS_PULSE_LED] = sys_pulse_led,
    [SYS_GET_FRAMEBUFFER] = sys_get_framebuffer,
    [SYS_GETC] = sys_getc,
};

#endif