#ifndef __ELF_H__
#define __ELF_H__

#include "macros.h"
#include "filesystem/filesystem.h"
#include "filesystem/disk.h"
#include "memory/kmalloc.h"
#include "system/process.h"
#include "memory/paging.h"
#include "memory/mmap.h"
#include "types/elf_types.h"

// 0x7f 'E' 'L' 'F', backwards for endianness
#define ELF_MAGIC   0x464c457f




void readelf(file_t* file);

#endif
