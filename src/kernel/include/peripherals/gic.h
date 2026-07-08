#ifndef __PERIPHERALS_GIC_H__
#define __PERIPHERALS_GIC_H__

#include "macros.h"
#include "types/gic_types.h"

#define GIC_BASE        0xFFFF8000FF840000



#define REGS_GICD ((struct gicd_regs*) (GIC_BASE + 0x1000))
#define REGS_GICC ((struct gicc_regs*) (GIC_BASE + 0x2000))
#endif
