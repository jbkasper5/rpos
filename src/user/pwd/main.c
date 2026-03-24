#include <stdint.h>
#include "uapi/rpos/syscall_macros.h"



extern uint64_t syscall(uint64_t sysnum, ...);

int main(){
    // write syscall is: uint64_t fd, uint64_t buf, uint64_t count
    syscall(SYS_WRITE, 0, "Executing 'pwd'...\n");
    syscall(SYS_EXIT_GROUP);
    return 0;
}