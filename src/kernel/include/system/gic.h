#ifndef __GIC_H__
#define __GIC_H__

#include <stdint.h>
#include "macros.h"

#define GICD_BASE       0xFF841000
#define GICC_BASE       0xFF842000

// Distributor Registers
#define GICD_CTRLR      ((volatile u32*)(GICD_BASE + 0x000))
#define GICD_IGROUPR    ((volatile u32*)(GICD_BASE + 0x080))
#define GICD_ISENABLER  ((volatile u32*)(GICD_BASE + 0x100))
#define GICD_IPRIORITYR ((volatile u8*)(GICD_BASE + 0x400))    
#define GICD_ITARGETSR  ((volatile u8*)(GICD_BASE + 0x800))

// CPU Interface Registers
#define GICC_CTRLR      ((volatile u32*)(GICC_BASE + 0x000))
#define GICC_PMR        ((volatile u32*)(GICC_BASE + 0x004))

#define IT_NR           128 



/*–––––––––––– GIC INTERRUPT CODES ––––––––––––*/
#define VIRT_TIMER              27
#define PHYS_TIMER              30
#define MINI_UART_RECV          125

#endif