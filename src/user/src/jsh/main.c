#include "jsh.h"

static void commandline(){
    int fd = open("/dev/ttyS1", O_RDWR);
    if(fd < 0){
        printf("open failed: %d\n", errno);   // musl returns -1 and sets errno, unlike the raw syscall
    }

    char buf[1024];
    char* cmdptr = (char*) &buf;
    while(true){
        read(fd, cmdptr, 1);
        char c = *cmdptr;
        if(c == '\r' || c == '\n'){
            *cmdptr = '\0';
            parse_command(buf);
            cmdptr = buf;
        }else if(c == 0x7F){
            // 0x7F is the backspace code
            cmdptr = MAX(cmdptr - 1, buf);
        }else{
            *cmdptr = c;
            cmdptr++;
        }
    }
}

int main(int argc, char** argv){
    printf("Made it to MUSL main()\n");

    printf("Argc: %d\n", argc);

    for(int i = 0; i < argc; i++){
        printf("arg%d: %s\n", i, argv[i]);
    }
    
    syscall(SYS_SCRATCH);

    int* test = (int*) malloc(sizeof(int) * 32);
    if(test){
        *test = 100;
        printf("Used malloc yippee (test = 0x%ls, *test = %d)\n", test, *test);   
    }else{
        printf("Malloc failed :_(\n");
    }

    commandline();
    syscall(SYS_EXIT_GROUP);
    return 0;
}