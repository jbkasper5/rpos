#include "uabi/rpos/syscall_macros.h"
#include "uabi/rpos/fb.h"

#include "macros.h"

extern int syscall(uint64_t, ...);

TEST_FN void user(){
    syscall(SYS_WRITE, NULL, "Opening framebuffer device...\n");

    int fd = syscall(SYS_OPEN, "/dev/fb");

    fb_var_screeninfo vinfo;
    // syscall(SYS_IOCTL, fd, FBIOGET_VSCREENINFO, &vinfo);

    // char buf[4096];
    // syscall(SYS_MMAP, fd, &buf);
    
    syscall(SYS_WRITE, NULL, "Back from open!\n");
    syscall(SYS_EXIT_GROUP);
}