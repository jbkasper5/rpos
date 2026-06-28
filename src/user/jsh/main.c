#include "jsh.h"
#include "uabi/rpos/syscall_macros.h"

int main(){
    syscall(SYS_EXIT_GROUP);
    return 0;
}