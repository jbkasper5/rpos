#include "cd.h"

int main(int argc, char** argv){
    if(argc > 1){
        printf("ERR: Too many arguments received. Expected <= 1, got %d.\n", argc);
        return -1;
    }
    if(!argv){
        printf("ERR: unexpected argv!\n");
        return -1;
    }

    int ret;

    if(argc){
        // if an argument is specified, change dir to that argument
        ret = chdir(*argv);
    }else{
        // if no argument is specified, it's asking to change to the root
        ret = chdir("/");
    }

    if(!ret){
        printf("ERR: Unexpected return from `chdir`: %d\n", ret);
    }

    return 0;
}