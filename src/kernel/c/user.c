#include "printf.h"
#include "timer.h"
#include "utils.h"

void do_user_things(){
    while(TRUE){
        printf("Doin user things at EL %d\n", get_el());
        timer_sleep(1000);
    }
}