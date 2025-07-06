#ifndef __PERIPHERAL_UART_H__
#define __PERIPHERAL_UART_H__

#include "peripherals/base.h"
#include "macros.h"

struct AuxRegs{
    reg32_t irq_status;
    reg32_t enables;
    reg32_t reserved[14];
    reg32_t mu_io;
    reg32_t mu_ier;
    reg32_t mu_iir;
    reg32_t mu_lcr;
    reg32_t mu_mcr;
    reg32_t mu_lsr; 
    reg32_t mu_msr; 
    reg32_t mu_scratch; 
    reg32_t mu_control;
    reg32_t mu_status;
    reg32_t mu_baud_rate;
};

#define REGS_AUX ((struct AuxRegs*)(PERIPHERAL_BASE + 0x00215000))
#endif