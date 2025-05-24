#ifndef __MACROS_H__
#define __MACROS_H__

#define UNSCALED_PTR_ADD(x, y) ((void*)(((char*)(x)) + (y)))
#define UNSCALED_PTR_SUB(x, y) ((void*)(((char*)(x)) - (y)))

#define ABS(x) ((x < 0) ? (-x) : (x))

#define TRUE            1
#define FALSE           0
#define NULL            0
#define NOP             __asm__("nop")

#endif