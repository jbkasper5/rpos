#ifndef __PERIPHERALS_AUX_H__
#define __PERIPHERALS_AUX_H__

#include "macros.h"
#include "peripherals/base.h"
#include "types/aux_types.h"

#define REGS_AUX_LO ((struct AuxRegs*)(PBASE_PHYS + 0x00215000))
#define REGS_AUX ((struct AuxRegs*)(PBASE + 0x00215000))


// 0xFE215000
#endif