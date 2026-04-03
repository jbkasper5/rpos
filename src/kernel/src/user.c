#include "uapi/rpos/syscall_macros.h"
#include "uapi/rpos/fb.h"
#include "uapi/rpos/ioctls.h"
#include "uapi/rpos/mman.h"
#include "uapi/rpos/fds.h"

#include "macros.h"

extern int syscall(uint64_t, ...);

#define SCREEN_SIZE_BYTES        (800 * 480 * 4)

TEST_FN void user(){
    syscall(SYS_WRITE, STDOUT, "Opening framebuffer device...\n");

    int fd = syscall(SYS_OPEN, "/dev/fb0");

    fb_var_screeninfo vinfo;
    syscall(SYS_IOCTL, fd, FBIOGET_VSCREENINFO, &vinfo);

    // address, length, prot, flags, fd, offset
    // uint32_t* framebuffer = syscall(SYS_MMAP, NULL, SCREEN_SIZE_BYTES, PROT_READ | PROT_WRITE, 0, fd, 0);

    // make the thing black
    // for(uint32_t i = 0; i < SCREEN_SIZE_BYTES; i++) framebuffer[i] = 0xFF000000;
    
    char buf[256];
    char c;
    char* cp = buf;

    while(TRUE){
        c = syscall(SYS_GETC);
        if(c == '\n' || c == '\r'){
            *cp = '\n';
            cp++;
            *cp = '\0';
            cp = buf;
            break;
        }else{
            *cp = c;
            cp++;
        }
    }

    syscall(SYS_WRITE, STDOUT, buf);

    syscall(SYS_EXIT_GROUP);
}