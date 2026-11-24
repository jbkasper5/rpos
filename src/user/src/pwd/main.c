#include <stdint.h>
#include "userlib.h"

int main(int argc, char** argv){
    printf("Argc: %d\n", argc);

    for(int i = 0; i < argc; i++){
        printf("arg%d: %s\n", i, argv[i]);
    }
    
    printf("idk somewhere\n");
    return 0;
}