#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "spinlock.h"
#include "utils/datastructures.h"
#include "synchronization/atomic.h"
#include "synchronization/wait.h"

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

/*
#define __MUTEX_INITIALIZER(lockname) \
		{ .owner = ATOMIC_LONG_INIT(0) \
		, .wait_lock = __RAW_SPIN_LOCK_UNLOCKED(lockname.wait_lock) \
		, .wait_list = LIST_HEAD_INIT(lockname.wait_list) \
		__DEBUG_MUTEX_INITIALIZER(lockname) \
		__DEP_MAP_MUTEX_INITIALIZER(lockname) }

#define DEFINE_MUTEX(mutexname) \
	struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)
*/

#define MUTEX_UNLOCKED 		0

mutex_t* mutex_init();
void mutex_acquire(mutex_t* mutex);
void mutex_release(mutex_t* mutex);

#endif