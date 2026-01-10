#include "io/kprintf.h"
#include "peripherals/timer.h"
#include "peripherals/irq.h"
#include "peripherals/aux.h"
#include "system/scheduler.h"
#include "utils/timer.h"

const uint32_t interval_1 = CLOCKHZ;
uint32_t curr_val_1 = 0;

const uint32_t interval_3 = CLOCKHZ / 4;
uint32_t curr_val_3 = 0;

pqnode_t heap_storage[MAX_PROCESSES];
pq_t sleep_timer_queue = {
    .heap = heap_storage,
    .size = MAX_PROCESSES,
    .items = 0
};

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

    PDEBUG("Timer 1 received.\n");
}

void handle_timer_3(){
    curr_val_3 += interval_3;
    REGS_TIMER->compare[3] = curr_val_3;
    REGS_TIMER->control_status |= SYS_TIMER_IRQ_3;

    PDEBUG("Timer 3 received.\n");
}

void handle_physical_timer(){
    prime_physical_timer();
    PDEBUG("Physical timer received.\n");
}

void handle_virtual_timer(){
    // pop node from top of the queue
    // priority contains the timer that just completed
    // element is the integer proclist index that finished the timer request
    pqnode_t node = pq_pop(&sleep_timer_queue);

    reschedule((uint64_t) node.element);

    if(sleep_timer_queue.items){
        PDEBUG("Items remaining in queue: %d\n", sleep_timer_queue.items);
        // prime the next request, if there is one
        prime_virtual_timer(pq_peek(&sleep_timer_queue).priority);
    }else{
        // otherwise, disable the timer until someone else makes the nanosleep syscall
        PDEBUG("No more sleep requests, clearing virtual timer. \n");
        clear_virtual_timer();
    }
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

void timer_nanosleep(uint64_t nanoseconds){
    // TODO: CLOCKHZ needs tuning, nanoseconds don't properly convert to seconds
    uint64_t timer_request = ((nanoseconds * CLOCKHZ) / 1000000000ULL) + read_virtual_timer();

    // priority = absolute timer request, element = active process
    pq_add(&sleep_timer_queue, timer_request, active_process);

    // in case the incoming request is less than that of the existing request, reprogram
    // the timer, since the queue always has the earliest deadline first

    timer_request = pq_peek(&sleep_timer_queue).priority;
    PDEBUG("Timer request value: 0x%x\n", timer_request);
    prime_virtual_timer(timer_request);
    deschedule();
}