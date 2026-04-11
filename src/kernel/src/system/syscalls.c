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

u64 handle_syscall(u64 x0, u64 x1, u64 x2, u64 x3, u64 x4, u64 x5, u64 syscall_number, u64 regfile){
    DEBUG("Syscall Number: %d\n", syscall_number);
    reglist_t* user_regs = (reglist_t*) regfile;

    if(syscall_table[syscall_number]){
        return syscall_table[syscall_number](x0, x1, x2, x3, x4, x5);
    }else{
        return -1;
    }
}

u64 sys_write(u64 fd, u64 buf, u64 count, u64 unused1, u64 unused2, u64 unused3){
    // write a buffer to a file descriptor
    // get fd
    if(proclist.proclist[active_process].fds[fd].file_ops->write){
        return proclist.proclist[active_process].fds[fd].file_ops->write(proclist.proclist[active_process].fds, buf, count);
    }
    return 0;
}

u64 sys_read(u64 fd, u64 buf, u64 count, u64 unused1, u64 unused2, u64 unused3){
    return 0;
}

u64 sys_nanosleep(u64 ns, u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5){
    // slep
    timer_nanosleep(ns);
    return 0;
}

u64 sys_clock_gettime(u64 clock, u64 kernel_timespec, u64 unused2, u64 unused3, u64 unused4, u64 unused5){
    return 0;
}

u64 sys_mmap(u64 addr, u64 len, u64 prot, u64 flags, u64 fd, u64 offset){
    // TODO: look up the requested memory via the FD
    // TODO: actually use the addr instead of ignoring it like a bum

    
    
    map_pages(frame.fb, va_to_pa(frame.fb), 376, MAP_KERNEL, (u64) L0_TABLE);
    return 0;
}

u64 sys_munmap(u64 addr, u64 len, u64 unused1, u64 unused2, u64 unused3, u64 unused4){
    // unmap a chunk of memory starting from addr
    return 0;
}

u64 sys_execve(u64 path, u64 argv, u64 envp, u64 unused1, u64 unused2, u64 unused3){
    char* p = (char*) path;
    // read binary from path
    // load binary into program memory
    // pass in arguments and environment variables
    // jump back to ELF-defined entry point
    // if invalid, return -1
    return (u64) p;
}

u64 sys_pulse_led(u64 pin_num, u64 turn_on, u64 unused1, u64 unused2, u64 unused3, u64 unused4){
    pulse(pin_num, !turn_on);
    return 0;
}

u64 sys_io_setup(u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5, u64 unused6){
    return 0;
}


u64 sys_exit_group(u64 status, u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5){
    INFO("Current running process number: %d\n", active_process);
    reap();
    return SYS_SUCCESS;
}

u64 sys_get_framebuffer(u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5, u64 unused6){
    // return frame.fb;
    return 0;
}


u64 sys_open(u64 path, u64 flags, u64 unused3, u64 unused4, u64 unused5, u64 unused6){
    return SYS_ERROR;
    if(check_vfs((char*) path)){
        // file to open is virtual, likely looking for a device
        return 3;
    }else{
        open((const char*) path, flags);
    }
    return SYS_ERROR;
}

u64 sys_ioctl(u64 fd, u64 cmd, u64 arg, u64 unused1, u64 unused2, u64 unused3){
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


u64 sys_getc(u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5, u64 unused6){
    char c = uart_getc();
    uart_putc(c);
    return c;
}


u64 sys_clone3(u64 cl_args, u64 size, u64 unused1, u64 unused2, u64 unused3, u64 unused4){
    return 0;
}


u64 sys_pipe2(u64 fd_rets, u64 flags, u64 unused1, u64 unused2, u64 unused3, u64 unused4){
    // allocate a pipe buffer of 64KiB (16 pages)
    u64* pipe = buddy_alloc(16 << PAGE_SHIFT);

    // create 2 new file descriptors from the process's file descriptor list
    // read

    u64* return_fds = (u64*)fd_rets;

    // return_fds[0] = read_end;
    // return_fds[1] = write_end;

    return 0;
}

u64 sys_fork(u64 unused1, u64 unused2, u64 unused3, u64 unused4, u64 unused5, u64 unused6){    
    // procalloc
    pcb_t* newproc = clone_active_proc();

    add_to_schedule(newproc);


    // set return value of the new process to be 0, to denote it as the child post-fork
    SET_PCB_REG_NUM(newproc, 0, 0);
    return 1;
}