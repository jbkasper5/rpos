#include "syscalls.h"
#include "printf.h"
#include "gpio.h"
#include "timer.h"
#include "macros.h"

uint64_t handle_syscall(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t syscall_number){
    if(syscall_table[syscall_number]){
        return syscall_table[syscall_number](x0, x1, x2, x3, x4, x5);
    }else{
        return -1;
    }
}

uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    // write a buffer to a file descriptor
    printf((char*) buf);
    return NULL;
}

uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    return NULL;
}

uint64_t sys_nanosleep(uint64_t ns, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5){
    // slep
    timer_nanosleep(ns);
    return 0;
}

uint64_t sys_clock_gettime(uint64_t clock, uint64_t kernel_timespec, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5){
    return 0;
}

uint64_t sys_mmap(uint64_t addr, uint64_t len, uint64_t prot, uint64_t flags, uint64_t fd, uint64_t offset){
    // attempt to map a section of virtual memory 
    // if cannot be mapped, return -1
    // if mapped, return the pointer (virtual) to the start address of the mapped range
    return NULL;
}

uint64_t sys_munmap(uint64_t addr, uint64_t len, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4){
    // unmap a chunk of memory starting from addr
    return NULL;
}

uint64_t sys_execve(uint64_t path, uint64_t argv, uint64_t envp, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    char* p = (char*) path;
    // read binary from path
    // load binary into program memory
    // pass in arguments and environment variables
    // jump back to ELF-defined entry point
    // if invalid, return -1
    return (uint64_t) p;
}

uint64_t sys_pulse_led(uint64_t pin_num, uint64_t turn_on, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4){
    if(turn_on){
        gpio_set_pin_high(pin_num);
    }else{
        gpio_set_pin_low(pin_num);
    }
    return 0;
}