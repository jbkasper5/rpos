#include "jsh.h"
#include "usermacros.h"
#include "userlib.h"

#define TRUE        1

static char path[1024] = "/bin/";


static void process_command(char* cmd){
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


static void commandline(){
    int fd = syscall(SYS_OPEN, "/dev/ttyS1");
    char buf[1024];
    char* cmdptr = (char*) &buf;
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

int main(){
    unsigned long brk = syscall(SYS_BRK, 0);
    printf("Old brk: 0x%x\n", brk);
    brk = syscall(SYS_BRK, (brk + 0x1000));
    printf("New brk: 0x%x\n", brk);

    char* buf = (char*) brk - 0x1000;
    buf[0] = 'H';
    buf[1] = 'e';
    buf[2] = 'l';
    buf[3] = 'l';
    buf[4] = 'o';
    buf[5] = '!';
    buf[6] = '\0';

    printf("Testing heap: '%s'\n", buf);
    commandline();
    syscall(SYS_EXIT_GROUP);
    return 0;
}