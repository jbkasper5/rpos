#include "syscalls.h"

uint64_t handle_syscall(uint64_t syscall_number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6){
    if(syscall_table[syscall_number]){
        return syscall_table[syscall_number](arg1, arg2, arg3, arg4, arg5, arg6);
    }else{
        return NULL;
    }
}

uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    return NULL;
}

uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    return NULL;
}