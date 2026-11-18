#include "jsh.h"

extern char path[1024];

static void probe(){
    DIR* dir = opendir(".");
    printf("dir: 0x%lx\n", (u64) dir);

    struct dirent* entry;

    errno = 0; // Reset errno before calling readdir to distinguish EOF from error
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    pid_t pid = fork();

    if(pid){
        printf("Parent! (child_pid = %d)\n", pid);
    }else{
        printf("Child! (got pid %d)\n", pid);
    }

    while(1);
}

void parse_command(char* cmd){


    probe();

    int pid = syscall(SYS_FORK);
    if(pid){
        // parent
        // BUG: somehow pid gets corrupted and passes itself in
        syscall(SYS_WAITID, pid);
    }else{
        syscall(SYS_NANOSLEEP, 1000000000);
        char* copy = cmd;
        char* destcopy = path + 5;
        while(*copy){
            *destcopy++ = *copy++;
        } 
        printf("Attempting to open binary '%s'\n", path);
        int fd = syscall(SYS_EXECVE, path);
        if(fd < 0){
            printf("Could not open file: %d\n", fd);
        }else{
            printf("Opened file at %d\n", fd);     
        }
        syscall(SYS_EXIT_GROUP);
    }
}