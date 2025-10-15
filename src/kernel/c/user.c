#include "syscall_macros.h"
#include "macros.h"

__attribute__((section(".uram.text")))


uint64_t syscall(uint64_t sysnum, ...);


void user_pulse(){
    return;
}

__attribute__((section(".uram.text")))
#define SYS_WRITE 64
void do_user_things(){
    // write system call takes filedescriptor, buffer, count as arguments
    char str[] = "Num: 0\n";
    for(int i = 0; i < 10; i++){
        // incrememnt the character 0 to 1, then 2, then 3, etc.
        syscall(SYS_WRITE, NULL, str);
        str[5] += 1;
    }

    while(TRUE){
        // sleep 1 second
        syscall(SYS_NANOSLEEP, 1000000000);

        // turn 17 on
        syscall(SYS_PULSE_LED, USER_PIN, TRUE);

        // sleep 1 second
        syscall(SYS_NANOSLEEP, 1000000000);

        // turn 17 off
        syscall(SYS_PULSE_LED, USER_PIN, FALSE);
    }
}