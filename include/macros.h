#ifndef __MACROS_H__
#define __MACROS_H__

#include <stdint.h>

#define TRUE    1
#define FALSE   0
#define NULL    0
typedef volatile unsigned int reg32_t;

#define NOP() __asm__("nop")


#endif