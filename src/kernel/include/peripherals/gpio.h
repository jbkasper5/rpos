#ifndef __PERIPHERALS_GPIO_H__
#define __PERIPHERALS_GPIO_H__

#include "macros.h"
#include "peripherals/base.h"
#include "types/gpio_regs_types.h"

#define REGS_GPIO_LO ((struct GpioRegs*)(PBASE_PHYS + 0x00200000))
#define REGS_GPIO    ((struct GpioRegs*)(PBASE + 0x00200000))

#endif
