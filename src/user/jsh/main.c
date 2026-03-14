#include "jsh.h"
#include "syscall_macros.h"

int main(){
    syscall(SYS_GET_FRAMEBUFFER);
    return 0;
}