#include "syscall_macros.h"
#include "macros.h"

#define URAM_FN __attribute__((section(".uram.text")))
#define BLIT 250000000

uint64_t syscall(uint64_t sysnum, ...);

URAM_FN void dot(){
    // on
    syscall(SYS_PULSE_LED, USER_PIN, TRUE);

    // sleep quarter second while on
    syscall(SYS_NANOSLEEP, BLIT);

    // turn 17 off
    syscall(SYS_PULSE_LED, USER_PIN, FALSE);

    // sleep 1 blit off
    syscall(SYS_NANOSLEEP, BLIT);
}

URAM_FN void dash(){
    // turn 17 on
    syscall(SYS_PULSE_LED, USER_PIN, TRUE);

    // sleep 3/4 second
    syscall(SYS_NANOSLEEP, (3 * BLIT));

    // turn 17 off
    syscall(SYS_PULSE_LED, USER_PIN, FALSE);

    // sleep 1 blit off
    syscall(SYS_NANOSLEEP, BLIT);
}

URAM_FN void pause(int blits){
    syscall(SYS_NANOSLEEP, blits * BLIT);
}

URAM_FN void morse_character(char c){
    switch(c){
        case 'a': case 'A': dot(); dash(); break;
        case 'b': case 'B': dash(); dot(); dot(); dot(); break;
        case 'c': case 'C': dash(); dot(); dash(); dot(); break;
        case 'd': case 'D': dash(); dot(); dot(); break;
        case 'e': case 'E': dot(); break;
        case 'f': case 'F': dot(); dot(); dash(); dot(); break;
        case 'g': case 'G': dash(); dash(); dot();
        case 'h': case 'H': dot(); dot(); dot(); dot(); break;
        case 'i': case 'I': dot(); dot(); break;
        case 'j': case 'J': dot(); dash(); dash(); dash(); break;
        case 'k': case 'K': dash(); dot(); dash(); break;
        case 'l': case 'L': dot(); dash(); dot(); dot(); break;
        case 'm': case 'M': dash(); dash(); break;
        case 'n': case 'N': dash(); dot(); break;
        case 'o': case 'O': dash(); dash(); dash(); break;
        case 'p': case 'P': dot(); dash(); dash(); dot(); break;
        case 'q': case 'Q': dash(); dash(); dot(); dash(); break;
        case 'r': case 'R': dot(); dash(); dot(); break;
        case 's': case 'S': dot(); dot(); dot(); break;
        case 't': case 'T': dash(); break;
        case 'u': case 'U': dot(); dot(); dash(); break;
        case 'v': case 'V': dot(); dot(); dot(); dash(); break;
        case 'w': case 'W': dot(); dash(); dash(); break;
        case 'x': case 'X': dash(); dot(); dot(); dash(); break;
        case 'y': case 'Y': dash(); dot(); dash(); dash(); break;
        case 'z': case 'Z': dash(); dash(); dot(); dot(); break;
        case ' ': pause(1); break;
    }
    pause(2);
}

URAM_FN void morse(char* str){
    while(*str) morse_character(*str++);
}

URAM_FN void user_pulse(){
    return;
}

URAM_FN void do_user_things(){
    // write system call takes filedescriptor, buffer, count as arguments
    char str[] = "Free Coffee\n";
    // for(int i = 0; i < 10; i++){
    //     // incrememnt the character 0 to 1, then 2, then 3, etc.
    //     syscall(SYS_WRITE, NULL, str);
    //     str[5] += 1;
    // }

    // char msg[] = "Coffee";

    while(TRUE){
        morse(str);
    }
}