#ifndef __MACROS_H__
#define __MACROS_H__

#include <stdint.h>
#include <stddef.h>

typedef volatile uint32_t reg32_t;
typedef volatile uint64_t reg64_t;
typedef uint8_t bool;
typedef uint16_t pid_t;

#ifdef DEBUG
#define PDEBUG(...) kprintf("[DEBUG] "); kprintf(__VA_ARGS__);
#else
#define PDEBUG(...)
#endif

#define INFO(...) kprintf("[INFO] "); kprintf(__VA_ARGS__);
#define WARNING(...) kprintf("[WARNING] "); kprintf(__VA_ARGS__);
#define ERROR(...) kprintf("[ERROR] "); kprintf(__VA_ARGS__);

#ifdef NULL
#undef NULL
#endif

#define NULL        0

#define FALSE       0
#define TRUE        1


#define DEBUG_PIN           17 // pi pin 11, ground pin 9
#define USER_PIN            18 // pi pin 12, ground pin 14
#define USER_PIN_2          11 // pi pin 23, ground pin 25


#define UNSCALED_POINTER_ADD(ptr, val)      ((void*)((char*)ptr + val))
#define UNSCALED_POINTER_SUB(ptr, val)      ((void*)((char*)ptr - val))


#define WFI()       __asm__("wfi");

#define PACKED      __attribute__((packed))

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

#endif