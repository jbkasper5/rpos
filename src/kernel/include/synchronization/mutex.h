#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "macros.h"
#include "spinlock.h"
#include "datastructures.h"

// linux mutex structure
struct mutex {
	u64		owner;
	raw_spinlock_t		wait_lock;
// #ifdef CONFIG_MUTEX_SPIN_ON_OWNER
// 	struct optimistic_spin_queue osq; /* Spinner MCS lock */
// #endif
	list_head_t	wait_list;
// #ifdef CONFIG_DEBUG_MUTEXES
// 	void			*magic;
// #endif
// #ifdef CONFIG_DEBUG_LOCK_ALLOC
// 	struct lockdep_map	dep_map;
// #endif
};

#define __MUTEX_INITIALIZER(lockname) \
		{ .owner = ATOMIC_LONG_INIT(0) \
		, .wait_lock = __RAW_SPIN_LOCK_UNLOCKED(lockname.wait_lock) \
		, .wait_list = LIST_HEAD_INIT(lockname.wait_list) \
		__DEBUG_MUTEX_INITIALIZER(lockname) \
		__DEP_MAP_MUTEX_INITIALIZER(lockname) }

#define DEFINE_MUTEX(mutexname) \
	struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)


#endif