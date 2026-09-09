#include "uabi/rpos/syscall_macros.h"
#include "uabi/rpos/fb.h"
#include "uabi/rpos/ioctls.h"
#include "uabi/rpos/mman.h"
#include "uabi/rpos/fds.h"

#include "macros.h"

extern int syscall(u64, ...);
static TEST_FN void commandline();
static TEST_FN void process_command(char* cmd);
static TEST_FN void i2c_test();
void printf(char* format_str, ...);
static const char path[1024] = "/bin/";

// Linux KVM
TEST_FN void user(){
    // register the keyboard
    // commandline();
    i2c_test();

    while(1);
}

static TEST_FN void i2c_test(){
    int fd = syscall(SYS_OPEN, "/dev/i2c");
    if(fd < 0){
        printf("Error: file not found: %d\n", fd);
        return;
    }

    syscall(SYS_IOCTL, fd, 0x0703, 0x08);

    // char buf[] = "Hello!\n";

    // syscall(SYS_WRITE, fd, buf, 7);


    int ang = 135;
    syscall(SYS_WRITE, fd, &ang, sizeof(int));
}

static TEST_FN void commandline(){
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
        // BUG: somehow pid gets corrupted and passes itself in
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
        if(fd < 0){
            printf("Could not open file: %d\n", fd);
        }else{
            printf("Opened file at %d\n", fd);     
        }
        syscall(SYS_EXIT_GROUP);
    }
}