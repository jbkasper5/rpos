#ifndef __MACROS_H__
#define __MACROS_H__

#include <stdint.h>

typedef volatile uint32_t reg32_t;
typedef uint8_t bool;

#define NULL        0
#define FALSE       0
#define TRUE        1


#define DEBUG_PIN           26
#define USER_PIN            17


#define UNSCALED_POINTER_ADD(ptr, val)      ((void*)((char*)ptr + val))
#define UNSCALED_POINTER_SUB(ptr, val)      ((void*)((char*)ptr - val))


#define WFI()       __asm__("wfi");

#endif