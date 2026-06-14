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
    printf("Starting mutex experiment...\n");
    int pid = syscall(SYS_FORK);
    if(pid){
        // parent
        while(TRUE) syscall(SYS_TEST_MUTEX);
    }else{
        // child
        syscall(SYS_NANOSLEEP, 10000000000);
        while(TRUE) syscall(SYS_TEST_MUTEX);
    }
}

static TEST_FN void process_command(char* cmd){
    printf("CMD: '%s'\n", cmd);
    // u64 pid = syscall(SYS_FORK);

    // char* cp = cmd;
    // while(*cp){
    //     if(*cp == ' '){
    //         cp++;
    //         break;
    //     }
    //     cp++;
    // }

    // // child
    // if(pid == 0){
    //     // cp naively points to the character after the first space, or a null terminator
    //     syscall(SYS_EXECVE, cmd, cp);
    // }else{
    //     // sys_waitpid
    //     // syscall()
    //     return;
    // }
}