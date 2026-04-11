#ifndef __MACROS_H__
#define __MACROS_H__

#include <stdint.h>
#include <stddef.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef volatile u32 reg32_t;
typedef volatile u64 reg64_t;
typedef u8 bool;
typedef u16 pid_t;

#ifdef DEBUG
#define DEBUG(...) kprintf("[\e[35mDEBUG\e[0m] "); kprintf(__VA_ARGS__);
#else
#define DEBUG(...)
#endif

#define INFO(...) kprintf("[\e[32mINFO\e[0m] "); kprintf(__VA_ARGS__);
#define WARNING(...) kprintf("[\e[33mWARNING\e[0m] "); kprintf(__VA_ARGS__);
#define ERROR(...) kprintf("[\e[31mERROR\e[0m] "); kprintf(__VA_ARGS__);

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
#define BOOT_FN     __attribute__((section(".text.boot")))
#define BOOT_DATA   __attribute__((section(".boot.data")))
#define BOOT_BSS    __attribute__((section(".boot.bss")))
#define BOOT_RODATA __attribute__((section(".boot.rodata")))

#define TEST_FN         __attribute__((section(".text.test")))
#define TEST_DATA       __attribute__((section(".test.data")))
#define TEST_BSS        __attribute__((section(".test.bss")))
#define TEST_RODATA     __attribute__((section(".test.rodata")))


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define ALIGN_UP(addr, align)   (((addr) + ((align) - 1)) & ~((align) - 1))

extern void panic();
extern u64 pa_to_va(u64 pa);
extern u64 va_to_pa(u64 va);

#endif