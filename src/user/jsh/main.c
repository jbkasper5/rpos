#include "jsh.h"
#include "uapi/rpos/syscall_macros.h"

int main(){
    syscall(SYS_EXIT_GROUP);
    return 0;
}