#include <scoped_lock.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include "../io/logger.h"

#define C_UTILS_LOG_LOCK(lock, level, format, ...) \
   c_utils_logger_log(lock->logger, level, "LOCK", format, NULL, lock->info.file, lock->info.line, lock->info.function, ##__VA_ARGS__)

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

/**
* Called to automatically unlock the passed c_utils_scoped_lock instance
* once it leaves the scope. This function gets called by the GCC or
* Clang compiler attribute.
*/
static void _auto_unlock(struct c_utils_scoped_lock **s_lock) {
   (*s_lock)->release((*s_lock));
}

static void *_bad_acquire1(struct c_utils_scoped_lock *s_lock) {
   C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "The underlying lock does not support a secondary locking mechanism.");
   return NULL + 1;
}

/*
   Mutex definitions
*/

static void *_destroy_scoped_lock_mutex(struct c_utils_scoped_lock *s_lock) {
   // TODO: Make use of events so if the mutex is already locked, we wait until it is finished to destroy
   int errcode = pthread_mutex_destroy(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_ERROR(s_lock->logger, "pthread_mutex_destroy: \"%s\"", strerror(errcode));
   

   free(s_lock->lock);
   return s_lock;
}

static void *_acquire_scoped_lock_mutex(struct c_utils_scoped_lock *s_lock) {
   int errcode = pthread_mutex_lock(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_mutex_lock: \"%s\"", strerror(errcode));
   else   
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Obtained lock...");
   

   return s_lock;
}

static void *_release_scoped_lock_mutex(struct c_utils_scoped_lock *s_lock) {
   C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Releasing lock...");

   int errcode = pthread_mutex_unlock(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_mutex_unlock: \"%s\"", strerror(errcode));
   

   return s_lock;
}

static void *scoped_lock_no_op(struct c_utils_scoped_lock *s_lock) {
   return NULL + 1;
}


struct c_utils_scoped_lock *c_utils_scoped_lock_mutex(pthread_mutex_t *lock, struct c_utils_logger *logger) {
   struct c_utils_scoped_lock *s_lock = calloc(1, sizeof(struct c_utils_scoped_lock));
   if (!s_lock) {
      C_UTILS_LOG_ASSERT(logger, "calloc: \"%s\"", strerror(errno));
      return NULL;
   }

   if (!lock) {
      C_UTILS_LOG_INFO(logger, "Passed lock was NULL, all locking operations set to a NO-OP!");
      s_lock->dispose = s_lock->acquire0 = s_lock->acquire1 = s_lock->release = scoped_lock_no_op;
   } else {
      s_lock->lock = lock;
      s_lock->dispose = _destroy_scoped_lock_mutex;
      s_lock->acquire0 = _acquire_scoped_lock_mutex;
      s_lock->acquire1 = _bad_acquire1;
      s_lock->release = _release_scoped_lock_mutex;
   }
   s_lock->logger = logger;

   return s_lock;
}

/*
   SpinLock Definitions
*/


static void *_destroy_scoped_lock_spinlock(struct c_utils_scoped_lock *s_lock) {
   // TODO: Make use of events so if the mutex is already locked, we wait until it is finished to destroy
   int errcode = pthread_spin_destroy(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_spin_destroy: \"%s\"", strerror(errcode));
   

   free(s_lock->lock);

   return s_lock;
}

static void *_acquire_scoped_lock_spinlock(struct c_utils_scoped_lock *s_lock) {
   int errcode = pthread_spin_lock(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_spin_lock: \"%s\"", strerror(errcode));
   else   
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Obtained lock...");
   

   return s_lock;
}

static void *_release_scoped_lock_spinlock(struct c_utils_scoped_lock *s_lock) {
   C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Releasing lock...");

   int errcode = pthread_spin_unlock(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_spin_unlock: \"%s\"", strerror(errcode));
   

   return s_lock;
}

struct c_utils_scoped_lock *c_utils_scoped_lock_spinlock(pthread_spinlock_t *lock, struct c_utils_logger *logger) {
   struct c_utils_scoped_lock *s_lock = calloc(1, sizeof(struct c_utils_scoped_lock));
   if (!s_lock) {
      C_UTILS_LOG_ASSERT(logger, "calloc: \"%s\"", strerror(errno));
      return NULL;
   }

   if (!lock) {
      C_UTILS_LOG_INFO(logger, "Passed lock was NULL, all locking operations set to a NO-OP!");
      s_lock->dispose = s_lock->acquire0 = s_lock->acquire1 = s_lock->release = scoped_lock_no_op;
   } else {
      s_lock->lock = lock;
      s_lock->dispose = _destroy_scoped_lock_mutex;
      s_lock->acquire0 = _acquire_scoped_lock_mutex;
      s_lock->acquire1 = _bad_acquire1;
      s_lock->release = _release_scoped_lock_mutex;
   }
   s_lock->logger = logger;

   return s_lock;
}