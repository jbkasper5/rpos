#ifndef __PERIPHERALS_IRQ_H__
#define __PERIPHERALS_IRQ_H__

#include "peripherals/base.h"
#include "macros.h"

enum vc_irqs{
    SYS_TIMER_IRQ_0 = (1 << 0),
    SYS_TIMER_IRQ_1 = (1 << 1),
    SYS_TIMER_IRQ_2 = (1 << 2),
    SYS_TIMER_IRQ_3 = (1 << 3),
    AUX_IRQ = (1 << 29),
};

struct bcm_irq_regs{
    volatile reg32_t irq0_pending_0;
    volatile reg32_t irq0_pending_1;
    volatile reg32_t irq0_pending_2;
    volatile reg32_t res0;
    volatile reg32_t irq0_enable_0;
    volatile reg32_t irq0_enable_1;
    volatile reg32_t irq0_enable_2;
    volatile reg32_t res1;
    volatile reg32_t irq0_disable_0;
    volatile reg32_t irq0_disable_1;
    volatile reg32_t irq0_disable_2;
};

#define REGS_BCMIRQ ((struct bcm_irq_regs*) (PBASE + 0x0000B200))
#endif