#include "gpio.h"

void uart_init(){
    // first thing to do is enable the mini uart
    *AUX_ENABLES = 0x1;

    // 3 means the uart operates in 8-bit mode
    *AUX_MU_LCR_REG = 0x3;

    // set baud rate?
    *AUX_MU_BAUD_REG = 270;    // 115200 baud
    return;
}