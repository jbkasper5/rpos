#ifndef __MUTEX_TYPES_H__
#define __MUTEX_TYPES_H__

#include "macros.h"
#include "types/spinlock_types.h"
#include "types/atomic_types.h"
#include "types/wait_types.h"

// linux mutex structure
typedef struct mutex {
	atomic_long_t		owner;
	raw_spinlock_t		wait_lock;
// #ifdef CONFIG_MUTEX_SPIN_ON_OWNER
// 	struct optimistic_spin_queue osq; /* Spinner MCS lock */
// #endif
	wait_queue_head_t	wait_list;
// #ifdef CONFIG_DEBUG_MUTEXES
// 	void			*magic;
// #endif
// #ifdef CONFIG_DEBUG_LOCK_ALLOC
// 	struct lockdep_map	dep_map;
// #endif
} mutex_t;

#endif
