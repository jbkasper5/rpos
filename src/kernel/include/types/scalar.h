#ifndef __SCALAR_TYPES_H__
#define __SCALAR_TYPES_H__

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

#endif
