#include "uapi/rpos/syscall_macros.h"
#include "uapi/rpos/fb.h"
#include "uapi/rpos/ioctls.h"
#include "uapi/rpos/mman.h"
#include "uapi/rpos/fds.h"

#include "macros.h"

extern int syscall(u64, ...);
static TEST_FN void process_command(char* cmd);

#define SCREEN_SIZE_BYTES        (800 * 480 * 4)

// Linux KVM
TEST_FN void user(){
    syscall(SYS_WRITE, STDOUT, "Opening framebuffer device...\n");

    int fd = syscall(SYS_OPEN, "/dev/fb0");

    fb_var_screeninfo vinfo;
    syscall(SYS_IOCTL, fd, FBIOGET_VSCREENINFO, &vinfo);

    // address, length, prot, flags, fd, offset
    // u32* framebuffer = syscall(SYS_MMAP, NULL, SCREEN_SIZE_BYTES, PROT_READ | PROT_WRITE, 0, fd, 0);

    // make the thing black
    // for(u32 i = 0; i < SCREEN_SIZE_BYTES; i++) framebuffer[i] = 0xFF000000;
    
    char buf[256];
    char c;
    char* cp = buf;

    u64 pid = syscall(SYS_FORK);

    while(TRUE){
        if(pid){
            syscall(SYS_NANOSLEEP, 10000000);
            syscall(SYS_WRITE, STDOUT, "PARENT\n");
        }else{
            syscall(SYS_NANOSLEEP, 50000000);
            syscall(SYS_WRITE, STDOUT, "CHILD\n");
        }
    }

    // while(TRUE){
    //     c = syscall(SYS_GETC);
    //     if(c == '\n' || c == '\r'){
    //         *cp = '\n';
    //         *++cp = '\0';
    //         cp = buf;
    //         process_command(buf);
    //     }else{
    //         *cp++ = c;
    //     }
    // }

    syscall(SYS_EXIT_GROUP);
}

static TEST_FN void process_command(char* cmd){
    u64 pid = syscall(SYS_FORK);

    char* cp = cmd;
    while(*cp){
        if(*cp == ' '){
            cp++;
            break;
        }
        cp++;
    }

    // child
    if(pid == 0){
        // cp naively points to the character after the first space, or a null terminator
        syscall(SYS_EXECVE, cmd, cp);
    }else{
        // sys_waitpid
        // syscall()
        return;
    }
}