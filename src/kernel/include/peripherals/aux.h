#ifndef __PERIPHERALS_AUX_H__
#define __PERIPHERALS_AUX_H__

#include "macros.h"
#include "peripherals/base.h"

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

#define REGS_AUX ((struct AuxRegs*)(PBASE + 0x00215000))


// 0xFE215000
#endif