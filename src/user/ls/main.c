#include <stdint.h>
#include "uabi/rpos/syscall_macros.h"
#include "userlib.h"

int main(){
    // write syscall is: u64 fd, u64 buf, u64 count
    // syscall(SYS_WRITE, 0, "fakedir1\nfakedir2\nfakedir3\n");
    // syscall(SYS_EXIT_GROUP);

    printf("fakedir1\nfakedir2\nfakedir3\n");
    return 0;
}