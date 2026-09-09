#ifndef __PERIPHERALS_BSC_H__
#define __PERIPHERALS_BSC_H__

#include "macros.h"
#include "peripherals/base.h"
#include "types/bsc_types.h"

#define REGS_BSC0 ((struct BSCRegs*)(PBASE + 0x00205000))
#define REGS_BSC1 ((struct BSCRegs*)(PBASE + 0x00804000))
#define REGS_BSC3 ((struct BSCRegs*)(PBASE + 0x00205600))
#define REGS_BSC4 ((struct BSCRegs*)(PBASE + 0x00205800))
#define REGS_BSC5 ((struct BSCRegs*)(PBASE + 0x00205a80))
#define REGS_BSC6 ((struct BSCRegs*)(PBASE + 0x00205c00))

#endif