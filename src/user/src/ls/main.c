#include <stdint.h>
#include "uabi/rpos/syscall_macros.h"
#include "userlib.h"

int main(int argc, char** argv){
    printf("Argc: %d\n", argc);

    for(int i = 0; i < argc; i++){
        printf("arg%d: %s\n", i, argv[i]);
    }



    printf("fakedir1\nfakedir2\nfakedir3\n");
    return 0;
}