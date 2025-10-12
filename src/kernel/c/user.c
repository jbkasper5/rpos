#include "printf.h"
#include "timer.h"
#include "utils.h"
#include "macros.h"
#include "gpio.h"

__attribute__((section(".uram.text")))
void user_pulse(){
    // while(TRUE){
    //     // printf("Doin user things at EL %d\n", get_el());
    //     printf("Supposedly in userland...\n");

    //     // turn off user pin
    //     pulse(USER_PIN, TRUE);

    //     // wait 1 sec
    //     timer_sleep(1000);

    //     // turn user pin on
    //     pulse(USER_PIN, FALSE);

    //     // wait 1 sec
    //     timer_sleep(1000);
    // }
    char str[] = "123456";
    printf(str);
    return;
}

__attribute__((section(".uram.text")))
void do_user_things(){
    // printf("Reached user mode in EL: %d\n", get_el());
    // turn user pin off cuz kernel turned it on
    user_pulse();
    while(TRUE);
}