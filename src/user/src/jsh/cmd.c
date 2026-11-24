#include "jsh.h"
#include <string.h>

static char path[256] = "/bin/";
static char cmd[1025];
static char* argbuf[64];

// static void probe(){
//     // DIR* dir = opendir(".");
//     DIR* dir = opendir("/bin");
//     printf("dir: 0x%lx\n", (u64) dir);

//     struct dirent* entry;

//     errno = 0; // Reset errno before calling readdir to distinguish EOF from error
//     while ((entry = readdir(dir)) != NULL) {
//         printf("%s\n", entry->d_name);
//     }

//     pid_t pid = fork();

//     if(pid){
//         printf("Parent! (child_pid = %d)\n", pid);
//     }else{
//         printf("Child! (got pid %d)\n", pid);
//     }

//     while(1);
// }

void parse_command(char* inc_cmd){
    // probe();
    pid_t pid = fork();
    if(pid){
        syscall(SYS_WAITID, pid);
    }else{
        // copy the full command into a mutable array
        u64 command_len = strlen(inc_cmd);
        memcpy(cmd, inc_cmd, command_len);

        // separate the string based on spaces, which will become the argv
        const char* sep = " ";
        char* token = strtok(cmd, sep);

        if(token){
            argbuf[0] = token;
            int n_tokens = 1;
            while((token = strtok(NULL, sep))){
                if(n_tokens < 63) argbuf[n_tokens++] = token;
            }
            argbuf[n_tokens] = '\0';
        }

        // first argument becomes the path of the binary to open
        char* copy = cmd;
        char* destcopy = path + 5;
        while(*copy){
            *destcopy++ = *copy++;
        }
        printf("Attempting to open binary '%s'\n", path);

        int fd = execvp(path, argbuf);
        if(fd < 0){
            printf("Could not open file: %d\n", fd);
        }else{
            printf("Opened file at %d\n", fd);     
        }
        syscall(SYS_EXIT_GROUP);
    }
}