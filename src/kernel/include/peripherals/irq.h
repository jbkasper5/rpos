#ifndef __PERIPHERALS_IRQ_H__
#define __PERIPHERALS_IRQ_H__

#include "peripherals/base.h"
#include "macros.h"
#include "types/irq_types.h"

#define REGS_BCMIRQ ((struct bcm_irq_regs*) (PBASE + 0x0000B200))
#endif
