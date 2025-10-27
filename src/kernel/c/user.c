#include "syscall_macros.h"
#include "macros.h"

#define URAM_FN __attribute__((section(".uram.text")))
#define BLIT 250000000

uint64_t syscall(uint64_t sysnum, ...);

URAM_FN void dot(int pin){
    // on
    syscall(SYS_PULSE_LED, pin, TRUE);

    // sleep quarter second while on
    syscall(SYS_NANOSLEEP, BLIT);

    // turn 17 off
    syscall(SYS_PULSE_LED, pin, FALSE);

    // sleep 1 blit off
    syscall(SYS_NANOSLEEP, BLIT);
}

URAM_FN void dash(int pin){
    // turn 17 on
    syscall(SYS_PULSE_LED, pin, TRUE);

    // sleep 3/4 second
    syscall(SYS_NANOSLEEP, (3 * BLIT));

    // turn 17 off
    syscall(SYS_PULSE_LED, pin, FALSE);

    // sleep 1 blit off
    syscall(SYS_NANOSLEEP, BLIT);
}

URAM_FN void pause(int blits){
    syscall(SYS_NANOSLEEP, blits * BLIT);
}

URAM_FN void morse_character(char c, int pin){
    switch(c){
        case 'a': case 'A': dot(pin); dash(pin); break;
        case 'b': case 'B': dash(pin); dot(pin); dot(pin); dot(pin); break;
        case 'c': case 'C': dash(pin); dot(pin); dash(pin); dot(pin); break;
        case 'd': case 'D': dash(pin); dot(pin); dot(pin); break;
        case 'e': case 'E': dot(pin); break;
        case 'f': case 'F': dot(pin); dot(pin); dash(pin); dot(pin); break;
        case 'g': case 'G': dash(pin); dash(pin); dot(pin);
        case 'h': case 'H': dot(pin); dot(pin); dot(pin); dot(pin); break;
        case 'i': case 'I': dot(pin); dot(pin); break;
        case 'j': case 'J': dot(pin); dash(pin); dash(pin); dash(pin); break;
        case 'k': case 'K': dash(pin); dot(pin); dash(pin); break;
        case 'l': case 'L': dot(pin); dash(pin); dot(pin); dot(pin); break;
        case 'm': case 'M': dash(pin); dash(pin); break;
        case 'n': case 'N': dash(pin); dot(pin); break;
        case 'o': case 'O': dash(pin); dash(pin); dash(pin); break;
        case 'p': case 'P': dot(pin); dash(pin); dash(pin); dot(pin); break;
        case 'q': case 'Q': dash(pin); dash(pin); dot(pin); dash(pin); break;
        case 'r': case 'R': dot(pin); dash(pin); dot(pin); break;
        case 's': case 'S': dot(pin); dot(pin); dot(pin); break;
        case 't': case 'T': dash(pin); break;
        case 'u': case 'U': dot(pin); dot(pin); dash(pin); break;
        case 'v': case 'V': dot(pin); dot(pin); dot(pin); dash(pin); break;
        case 'w': case 'W': dot(pin); dash(pin); dash(pin); break;
        case 'x': case 'X': dash(pin); dot(pin); dot(pin); dash(pin); break;
        case 'y': case 'Y': dash(pin); dot(pin); dash(pin); dash(pin); break;
        case 'z': case 'Z': dash(pin); dash(pin); dot(pin); dot(pin); break;
        case ' ': pause(1); break;
    }
    pause(2);
}

URAM_FN void morse(char* str, int pin){
    while(*str) morse_character(*str++, pin);
}


URAM_FN void do_user_things(){
    // write system call takes filedescriptor, buffer, count as arguments
    char str[] = "Free Coffee\n";
    while(TRUE){
        syscall(SYS_WRITE, NULL, str);
        morse(str, USER_PIN);
    }
}

URAM_FN void do_user_things_2(){
    // write system call takes filedescriptor, buffer, count as arguments
    char str[] = "Free Snacks\n";
    while(TRUE){
        syscall(SYS_WRITE, NULL, str);
        morse(str, USER_PIN_2);
    }
}