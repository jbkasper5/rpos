#include "syscalls.h"
#include "printf.h"
#include "gpio.h"
#include "timer.h"

uint64_t handle_syscall(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t syscall_number){
    if(syscall_table[syscall_number]){
        return syscall_table[syscall_number](x0, x1, x2, x3, x4, x5);
    }else{
        return NULL;
    }
}

uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    printf((char*) buf);
    return NULL;
}

uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    return NULL;
}

uint64_t sys_nanosleep(uint64_t ns, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5){
    timer_sleep(ns / 1000000);
    return 0;
}

uint64_t sys_mmap(uint64_t addr, uint64_t len, uint64_t prot, uint64_t flags, uint64_t fd, uint64_t offset){
    return NULL;
}

uint64_t sys_munmap(uint64_t addr, uint64_t len, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4){
    return NULL;
}

uint64_t sys_pulse_led(uint64_t pin_num, uint64_t turn_on, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4){
    if(turn_on){
        gpio_set_pin_high(pin_num);
    }else{
        gpio_set_pin_low(pin_num);
    }
    return 0;
}