#ifndef __CACHE_H__
#define __CACHE_H__

#include "macros.h"
#include "types/scalar.h"

void dcache_clean_range(u64 start, u64 end);
void dcache_clean_invalidate_range(u64 start, u64 end);
void dcache_invalidate_range(u64 start, u64 end);


#endif