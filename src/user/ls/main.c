#include <stdint.h>

#define NULL        (void*) 0
#define SYS_WRITE   64



extern uint64_t syscall(uint64_t sysnum, ...);

char str[] = "Hello from userland!\n";

int main(){
    // write syscall is: uint64_t fd, uint64_t buf, uint64_t count
    syscall(SYS_WRITE, NULL, str);
    return 0;
}