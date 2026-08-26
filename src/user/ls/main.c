#include <stdint.h>
#include "uabi/rpos/syscall_macros.h"

extern uint64_t syscall(uint64_t sysnum, ...);

int main(){
    // write syscall is: u64 fd, u64 buf, u64 count
    syscall(SYS_WRITE, 0, "fakedir1\nfakedir2\nfakedir3\n");
    syscall(SYS_EXIT_GROUP);
    return 0;
}