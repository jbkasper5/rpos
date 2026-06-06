#include "uapi/rpos/syscall_macros.h"
#include "uapi/rpos/fb.h"
#include "uapi/rpos/ioctls.h"
#include "uapi/rpos/mman.h"
#include "uapi/rpos/fds.h"

#include "macros.h"

extern int syscall(u64, ...);
static TEST_FN void process_command(char* cmd);
void printf(char* format_str, ...);

#define SCREEN_SIZE_BYTES        (800 * 480 * 4)

// Linux KVM
TEST_FN void user(){

    int fd = syscall(SYS_OPEN, "/dev/ttyS1");
    char buf[1024];
    // child
    char* bufptr = buf;
    while(TRUE){

        // blocking read frmo the keyboard device
        syscall(SYS_READ, fd, bufptr, 1);

        // enter key is 0xD or 13 (aka '\r')
        if(*bufptr == '\r'){
            *bufptr = '\0';
            process_command(buf);
            bufptr = buf;

        // 0x7F is backspace
        }else if(*bufptr == 0x7F){

            // move the pointer back one
            bufptr = MAX(bufptr - 1, buf);
        }else{
            // otherwise get ready to receive next character
            bufptr++;
        }
    }
}


static TEST_FN void process_command(char* cmd){
    printf("CMD: '%s'\n", cmd);
    u64 pid = syscall(SYS_FORK);

    if(pid){
        // parent
        syscall(SYS_WAITID, pid);


    }else{
        // child

        // should be something like that once things are set up properly
        // syscall(SYS_EXECVE, cmd);

        // reap the child process once it's done doing it's thing (technically shouldn't need to do this since it'll EXEC)
        syscall(SYS_EXIT_GROUP);
    }
}