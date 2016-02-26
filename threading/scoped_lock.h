#ifndef MU_SCOPED_LOCK_H
#define MU_SCOPED_LOCK_H

#include <pthread.h>
#include <stdbool.h>

struct c_utils_scoped_lock;

#ifdef NO_C_UTILS_PREFIX
typedef struct c_utils_scoped_lock scoped_lock_t;
#define scoped_lock_mutex(...) c_utils_scoped_lock_mutex(__VA_ARGS__)
#define scoped_lock_spinlock(...) c_utils_scoped_lock_spinlock(__VA_ARGS__)
#endif


struct c_utils_scoped_lock *c_utils_scoped_lock_mutex(pthread_mutex_t *lock, struct c_utils_logger *logger);


struct c_utils_scoped_lock *c_utils_scoped_lock_spinlock(pthread_spinlock_t *lock, struct c_utils_logger *logger);

// TODO: Implement!
void c_utils_scoped_lock_destroy(struct c_utils_scoped_lock *lock);


#define SCOPED_LOCK_FROM(lock, logger) _Generic((lock), pthread_mutex_t *: c_utils_scoped_lock_mutex, pthread_spinlock_t *: c_utils_scoped_lock_spinlock)(lock, logger)

/*
   Note how we create a temporary variable to point to the
   passed lock. This is because it needs to be defined
   inside of the scope in order to have the cleanup function be
   called.
*/ 

#define SCOPE_AUTO_UNLOCK __attribute__ ((__cleanup__(_auto_unlock)))

#define _SCOPED_LOCK(lock, n) \
   lock->info.line = __LINE__; \
   lock->info.file = __FILE__; \
   lock->info.function = __FUNCTION__; \
   for(struct c_utils_scoped_lock *tmp_lock SCOPE_AUTO_UNLOCK = s_lock, *_test = tmp_lock->acquire ##n (tmp_lock); _test; _test = NULL)

#define SCOPED_LOCK0(lock) _SCOPED_LOCK(lock, 0)

#define SCOPED_LOCK1(lock) _SCOPED_LOCK(lock, 1)

#define SCOPED_LOCK(s_lock) SCOPED_LOCK0(s_lock)

#endif /* MU_SCOPED_LOCK_H */