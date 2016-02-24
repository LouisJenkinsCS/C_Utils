#ifndef MU_SCOPED_LOCK_H
#define MU_SCOPED_LOCK_H

#include <pthread.h>
#include <stdbool.h>

#ifdef C_UTILS_USE_POSIX_STD
#define SCOPED_LOCK(...) MU_SCOPED_LOCK(__VA_ARGS__)
#define SCOPED_LOCK0(...) MU_SCOPED_LOCK0(__VA_ARGS__)
#define SCOPED_LOCK1(...) MU_SCOPED_LOCK1(__VA_ARGS__)
#define SCOPED_LOCK_FROM(...) MU_SCOPED_LOCK_FROM(__VA_ARGS__)
#define scoped_lock_t MU_Scoped_Lock_t
#define scoped_lock_mutex(...) MU_Scoped_Lock_mutex(__VA_ARGS__)
#define scoped_lock_spinlock(...) MU_Scoped_Lock_spinlock(__VA_ARGS__)
#endif

typedef struct c_utils_scoped_lock MU_Scoped_Lock_t;


MU_Scoped_Lock_t *MU_Scoped_Lock_mutex(pthread_mutex_t *lock);


MU_Scoped_Lock_t *MU_Scoped_Lock_spinlock(pthread_spinlock_t *lock);


#define MU_SCOPED_LOCK_FROM(lock) _Generic((lock), pthread_mutex_t *: MU_Scoped_Lock_mutex, pthread_spinlock_t *: MU_Scoped_Lock_spinlock)(lock)

/*
   Note how we create a temporary variable to point to the
   passed lock. This is because it needs to be defined
   inside of the scope in order to have the cleanup function be
   called.
*/ 

#define MU_SCOPE_AUTO_UNLOCK __attribute__ ((__cleanup__(_auto_unlock)))

#define _MU_SCOPED_LOCK(lock, n) for(MU_Scoped_Lock_t *tmp_lock MU_SCOPE_AUTO_UNLOCK = s_lock, *_test = tmp_lock->acquire ##n (tmp_lock); _test; _test = NULL)

#define MU_SCOPED_LOCK0(lock) _MU_SCOPED_LOCK(lock, 0)

#define MU_SCOPED_LOCK1(lock) _MU_SCOPED_LOCK(lock, 1)

#define MU_SCOPED_LOCK(s_lock) MU_SCOPED_LOCK0(s_lock)

#endif /* MU_SCOPED_LOCK_H */