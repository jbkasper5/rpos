#ifndef __VIRTUAL_MEMORY_TYPES_H__
#define __VIRTUAL_MEMORY_TYPES_H__

#include "macros.h"
#include "utils/datastructures.h"

typedef enum {
    VMA_READ       = (1UL << 0),
    VMA_WRITE      = (1UL << 1),
    VMA_EXEC       = (1UL << 2),
    VMA_USER       = (1UL << 3),
    VMA_KERNEL     = (1UL << 4),
    VMA_DEVICE     = (1UL << 5),
    VMA_NOCACHE    = (1UL << 6),
    VMA_CACHE      = (1UL << 7),
    VMA_WRITE_COMB = (1UL << 8)
} VMA_PERMISSIONS;

// virtual memory area struct
typedef struct {
    // TODO: can optimize memory by storing the permission bits in the bottom bits of the 
    // virtal addresses, since they must be page aligned
    u64 start;
    u64 end;
    list_head_t list;
    u8 permissions;
} vma;

#endif