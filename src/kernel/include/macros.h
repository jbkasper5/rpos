#ifndef __MACROS_H__
#define __MACROS_H__

#include <stdint.h>

typedef volatile uint32_t reg32_t;
typedef uint8_t bool;

#define NULL        0
#define FALSE       0
#define TRUE        1


#define WFI()       __asm__("wfi");

#endif