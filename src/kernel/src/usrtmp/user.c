#include "uabi/rpos/syscall_macros.h"
#include "uabi/rpos/fb.h"
#include "uabi/rpos/ioctls.h"
#include "uabi/rpos/mman.h"
#include "uabi/rpos/fds.h"

#include "macros.h"

extern int syscall(u64, ...);
static TEST_FN void process_command(char* cmd);
void printf(char* format_str, ...);
static const char path[1024] = "/bin/";

// Linux KVM
TEST_FN void user(){
    // register the keyboard
    int fd = syscall(SYS_OPEN, "/dev/ttyS1");
    char buf[1024];
    char* cmdptr = &buf;
    while(TRUE){
        syscall(SYS_READ, fd, cmdptr, 1);
        char c = *cmdptr;
        if(c == '\r' || c == '\n'){
            *cmdptr = '\0';
            process_command(buf);
            cmdptr = buf;
        }else if(c == 0x7F){
            // 0x7F is the backspace code
            cmdptr = MAX(cmdptr - 1, buf);
        }else{
            *cmdptr = c;
            cmdptr++;
        }
    }
}


static TEST_FN void process_command(char* cmd){
    int pid = syscall(SYS_FORK);
    if(pid){
        // parent
        syscall(SYS_WAITID, pid);
    }else{
        syscall(SYS_NANOSLEEP, 1000000000);
        char* copy = cmd;
        char* destcopy = path + 5;
        while(*copy){
            *destcopy++ = *copy++;
        } 
        printf("Attempting to open binary '%s'\n", path);
        int fd = syscall(SYS_EXECVE, path);
        printf("Opened file at %d\n", fd);
        syscall(SYS_EXIT_GROUP);
    }
}