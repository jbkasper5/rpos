#ifndef __PERIPHERALS_BASE_H__
#define __PERIPHERALS_BASE_H__

#ifdef KERNEL_VM
#define PBASE      0xFFFF0000FE000000UL  // virtual MMIO base
#else
#define PBASE      0xFE000000UL          // physical MMIO base
#endif

#define CORE_CLOCK_SPEED    1500000000

#endif