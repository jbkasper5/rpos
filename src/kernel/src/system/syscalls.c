#include "system/syscalls.h"
#include "system/scheduler.h"
#include "io/kprintf.h"
#include "io/gpio.h"
#include "utils/timer.h"
#include "macros.h"
#include "io/lcd.h"
#include "memory/mem.h"
#include "uapi/rpos/ioctls.h"
#include "uapi/rpos/fb.h"
#include "memory/mmap.h"

uint64_t handle_syscall(uint64_t x0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t syscall_number){
    DEBUG("Syscall Number: %d\n", syscall_number);
    if(syscall_table[syscall_number]){
        return syscall_table[syscall_number](x0, x1, x2, x3, x4, x5);
    }else{
        return -1;
    }
}

uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    // write a buffer to a file descriptor
    kprintf((char*) buf);
    return 0;
}

uint64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    return 0;
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
    // TODO: look up the requested memory via the FD
    // TODO: actually use the addr instead of ignoring it like a bum

    
    
    map_pages(frame.fb, va_to_pa(frame.fb), 376, MAP_KERNEL, (uint64_t) L0_TABLE);
    return 0;
}

uint64_t sys_munmap(uint64_t addr, uint64_t len, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4){
    // unmap a chunk of memory starting from addr
    return 0;
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
    pulse(pin_num, !turn_on);
    return 0;
}

uint64_t sys_io_setup(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5, uint64_t unused6){
    return 0;
}


uint64_t sys_exit_group(uint64_t status, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5){
    INFO("Current running process number: %d\n", active_process);
    reap();
    return SYS_SUCCESS;
}

uint64_t sys_get_framebuffer(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5, uint64_t unused6){
    // return frame.fb;
    return 0;
}


uint64_t sys_open(uint64_t path, uint64_t flags, uint64_t unused3, uint64_t unused4, uint64_t unused5, uint64_t unused6){
    if(check_vfs((char*) path)){
        // file to open is virtual, likely looking for a device
        return 3;
    }else{
        open((const char*) path, flags);
    }
    return SYS_ERROR;
}

uint64_t sys_ioctl(uint64_t fd, uint64_t cmd, uint64_t arg, uint64_t unused1, uint64_t unused2, uint64_t unused3){
    INFO("Handling IOCTL Request: fd=%d, cmd=0x%x, arg=0x%x\n", fd, cmd, arg);
    switch(cmd){
        case FBIOGET_VSCREENINFO: 
            fb_var_screeninfo test = {
                .bits_per_pixel = 10
            };
            memcpy((void*) arg, &test, sizeof(fb_var_screeninfo));
            return SYS_SUCCESS;
        case FBIOPUT_VSCREENINFO: break;
        case FBIOGET_FSCREENINFO: break;
        case FBIOGETCMAP: break;
        case FBIOPUTCMAP: break;
        case FBIOPAN_DISPLAY: break;
        default:
            return SYS_ERROR;
    }
    return SYS_ERROR;
}


uint64_t sys_getc(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5, uint64_t unused6){
    return uart_getc();
}