#ifndef __MACROS_H__
#define __MACROS_H__

#include "types/scalar.h"

// decomissioned
// double seconds_since_boot();

u64 usec_since_boot();
u64 ticks_since_boot();

#define LOG(color, label, ...) do {                                   \
    u64 _ts = usec_since_boot();                                      \
    kprintf("[\e[" color "m" label " %d.%06ds\e[0m] ",                \
            _ts / 1000000, _ts % 1000000);                            \
    kprintf(__VA_ARGS__);                                             \
} while(0)

#ifdef DEBUG
#define DEBUG(...)   LOG("35", "DEBUG",   __VA_ARGS__)
#else
#define DEBUG(...)
#endif
#define INFO(...)    LOG("32", "INFO",    __VA_ARGS__)
#define WARNING(...) LOG("33", "WARNING", __VA_ARGS__)
#define ERROR(...)   LOG("31", "ERROR",   __VA_ARGS__)

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


#define WFI()       asm volatile("wfi")
#define PACKED      __attribute__((packed))
#define BOOT_FN     __attribute__((section(".text.boot")))
#define BOOT_DATA   __attribute__((section(".boot.data")))
#define BOOT_BSS    __attribute__((section(".boot.bss")))
#define BOOT_RODATA __attribute__((section(".boot.rodata")))

#define TEST_FN         __attribute__((section(".text.test")))
#define TEST_DATA       __attribute__((section(".test.data")))
#define TEST_BSS        __attribute__((section(".test.bss")))
#define TEST_RODATA     __attribute__((section(".test.rodata")))

#define INTERRUPT_ENABLE()        asm volatile("msr daifclr, #0xf")
#define INTERRUPT_DISABLE()       asm volatile("msr daifset, #0xf")

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define ALIGN_UP(addr, align)   (((addr) + ((align) - 1)) & ~((align) - 1))

#define READ_SYSREG(reg) ({                                  \
    u64 _v;                                                  \
    __asm__ volatile("mrs %0, " #reg : "=r"(_v));            \
    _v;                                                      \
})

extern void panic();
extern u64 pa_to_va(u64 pa);
extern u64 va_to_pa(u64 va);

#endif