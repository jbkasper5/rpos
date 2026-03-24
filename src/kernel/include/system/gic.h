#ifndef __GIC_H__
#define __GIC_H__

#include <stdint.h>

#define GICD_BASE       0xFF841000
#define GICC_BASE       0xFF842000

// Distributor Registers
#define GICD_CTRLR      ((volatile uint32_t*)(GICD_BASE + 0x000))
#define GICD_IGROUPR    ((volatile uint32_t*)(GICD_BASE + 0x080))
#define GICD_ISENABLER  ((volatile uint32_t*)(GICD_BASE + 0x100))
#define GICD_IPRIORITYR ((volatile uint8_t*)(GICD_BASE + 0x400))    
#define GICD_ITARGETSR  ((volatile uint8_t*)(GICD_BASE + 0x800))

// CPU Interface Registers
#define GICC_CTRLR      ((volatile uint32_t*)(GICC_BASE + 0x000))
#define GICC_PMR        ((volatile uint32_t*)(GICC_BASE + 0x004))

#define IT_NR           128 



/*–––––––––––– GIC INTERRUPT CODES ––––––––––––*/
#define VIRT_TIMER              27
#define PHYS_TIMER              30
#define MINI_UART_RECV          125

#endif