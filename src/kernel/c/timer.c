#include "print.h"
#include "peripherals/timer.h"
#include "peripherals/irq.h"
#include "peripherals/aux.h"

const uint32_t interval_1 = CLOCKHZ;
uint32_t curr_val_1 = 0;

const uint32_t interval_3 = CLOCKHZ / 4;
uint32_t curr_val_3 = 0;

void timer_init(){
    curr_val_1 = REGS_TIMER->counter_lo;
    curr_val_1 += interval_1;
    REGS_TIMER->compare[1] = curr_val_1;

    curr_val_3 = REGS_TIMER->counter_lo;
    curr_val_3 += interval_3;
    REGS_TIMER->compare[3] = curr_val_3;
}

void handle_timer_1(){
    curr_val_1 += interval_1;
    REGS_TIMER->compare[1] = curr_val_1;
    REGS_TIMER->control_status |= SYS_TIMER_IRQ_1;

    printf("Timer 1 received.\n");
}

void handle_timer_3(){
    curr_val_3 += interval_3;
    REGS_TIMER->compare[3] = curr_val_3;
    REGS_TIMER->control_status |= SYS_TIMER_IRQ_3;

    printf("Timer 3 received.\n");
}

uint64_t timer_get_ticks(){
    uint32_t hi = REGS_TIMER->counter_hi;
    uint32_t lo = REGS_TIMER->counter_lo;

    if(hi != REGS_TIMER->counter_hi){
        hi = REGS_TIMER->counter_hi;
        lo = REGS_TIMER->counter_lo;
    }

    return ((uint64_t) hi << 32) | lo;
}

void timer_sleep(uint32_t milliseconds){
    uint64_t start = timer_get_ticks();
    while(timer_get_ticks() < start + (milliseconds * 1000));
}