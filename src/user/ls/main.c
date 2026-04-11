#include <stdint.h>
#include "uapi/rpos/syscall_macros.h"

extern u64 syscall(u64 sysnum, ...);

int main(){
    // write syscall is: u64 fd, u64 buf, u64 count
    syscall(SYS_WRITE, 0, "Executing 'ls'...\n");
    syscall(SYS_EXIT_GROUP);
    return 0;
}