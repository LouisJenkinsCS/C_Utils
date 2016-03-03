#ifndef MU_SCOPED_LOCK_H
#define MU_SCOPED_LOCK_H

#include <pthread.h>
#include <stdbool.h>
#include "../io/logger.h"

struct c_utils_scoped_lock
{
   // The instance of lock.
   void *lock;
   // Keeps track of information needed to log any possible issues.
   struct {
      const char *line;
      const char *file;
      const char *function;
   } info;
   // Logger used to log debug information and errors.
   struct c_utils_logger *logger;
   // For normal locks, I.E Mutex and Spinlock
   void *(*acquire0)(struct c_utils_scoped_lock *);
   // For locks with types of locking mechanisms, I.E RWLocks
   void *(*acquire1)(struct c_utils_scoped_lock *);
   // For when we exit the scope
   void *(*release)(struct c_utils_scoped_lock *);
   // For when the user frees this, the lock gets freed too.
   void *(*dispose)(struct c_utils_scoped_lock *);
};

#ifdef NO_C_UTILS_PREFIX
typedef struct c_utils_scoped_lock scoped_lock_t;
#define scoped_lock_mutex(...) c_utils_scoped_lock_mutex(__VA_ARGS__)
#define scoped_lock_spinlock(...) c_utils_scoped_lock_spinlock(__VA_ARGS__)
#endif


struct c_utils_scoped_lock *c_utils_scoped_lock_mutex_from(pthread_mutex_t *lock, struct c_utils_logger *logger);

struct c_utils_scoped_lock *c_utils_scoped_lock_mutex(pthread_mutexattr_t *attr, struct c_utils_logger *logger);

struct c_utils_scoped_lock *c_utils_scoped_lock_spinlock_from(pthread_spinlock_t *lock, struct c_utils_logger *logger);

struct c_utils_scoped_lock *c_utils_scoped_lock_spinlock(int flags, struct c_utils_logger *logger);

struct c_utils_scoped_lock *c_utils_scoped_lock_rwlock_from(pthread_rwlock_t *lock, struct c_utils_logger *logger);

struct c_utils_scoped_lock *c_utils_scoped_lock_rwlock(pthread_rwlockattr_t *attr, struct c_utils_logger *logger);

struct c_utils_scoped_lock *c_utils_scoped_lock_no_op();

/**
* Called to automatically unlock the passed c_utils_scoped_lock instance
* once it leaves the scope. This function gets called by the GCC or
* Clang compiler attribute.
*/
void c_utils_auto_unlock(struct c_utils_scoped_lock **s_lock);


// TODO: Implement!
void c_utils_scoped_lock_destroy(struct c_utils_scoped_lock *lock);

#define C_UTILS_SCOPED_LOCK_ERR_MSG "Was unable to allocate the scoped_lock, please check logs!!!"

#define C_UTILS_SCOPED_LOCK_FROM(lock, logger) _Generic((lock), \
      pthread_mutex_t *: c_utils_scoped_lock_mutex_from, \
      pthread_spinlock_t *: c_utils_scoped_lock_spinlock_from, \
      pthread_rwlock_t *: c_utils_scoped_lock_rwlock_from)(lock, logger)

#define C_UTILS_SCOPE_AUTO_UNLOCK __attribute__ ((__cleanup__(c_utils_auto_unlock)))

/*
   Note how we create a temporary variable to point to the
   passed lock. This is because it needs to be defined
   inside of the scope in order to have the cleanup function be
   called.
*/ 
#define _C_UTILS_SCOPED_LOCK(s_lock, n) \
   s_lock->info.line = C_UTILS_STRINGIFY(__LINE__); \
   s_lock->info.file = __FILE__; \
   s_lock->info.function = __FUNCTION__; \
   for (struct c_utils_scoped_lock *tmp_lock C_UTILS_SCOPE_AUTO_UNLOCK = s_lock, *_test = tmp_lock->acquire ##n (tmp_lock); _test; _test = NULL)

#define C_UTILS_SCOPED_LOCK0(s_lock) _C_UTILS_SCOPED_LOCK(s_lock, 0)

#define C_UTILS_SCOPED_LOCK1(s_lock) _C_UTILS_SCOPED_LOCK(s_lock, 1)

#define C_UTILS_SCOPED_LOCK(s_lock) C_UTILS_SCOPED_LOCK0(s_lock)

#define C_UTILS_UNACCESSIBLE do { __builtin_unreachable(); assert(0 && "Reach unreachable block!"); } while(0)

#endif /* MU_SCOPED_LOCK_H */