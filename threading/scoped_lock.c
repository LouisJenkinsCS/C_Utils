#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#include "scoped_lock.h"
#include "../misc/alloc_check.h"
#include "../io/logger.h"

/*
   Used when we failed to acquire a lock. We cannot use the below because the scoped_lock's info is outdated.
*/
#define C_UTILS_LOG_LOCK_FAILURE(logger, log_info, format, ...) \
   c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ASSERTION, "LOCK", format, NULL, log_info.file, log_info.line, log_info.function, ##__VA_ARGS__)

/*
   Used mostly for trace logging or errors while dealing with the underlying lock.
*/
#define C_UTILS_LOG_LOCK(lock, level, format, ...) \
   c_utils_logger_log(lock->logger, level, "LOCK", format, NULL, lock->log_info.file, lock->log_info.line, lock->log_info.function, ##__VA_ARGS__)

/**
* Called to automatically unlock the passed c_utils_scoped_lock instance
* once it leaves the scope. This function gets called by the GCC or
* Clang compiler attribute.
*/
void c_utils_auto_unlock(struct c_utils_scoped_lock **s_lock) {
   (*s_lock)->release((*s_lock));
}

static void *_bad_acquire1(struct c_utils_scoped_lock *s_lock, struct c_utils_scoped_lock_log_info info) {
   C_UTILS_LOG_LOCK_FAILURE(s_lock->logger, info, "The underlying lock does not support a secondary locking mechanism.");

   exit(EXIT_FAILURE);
}

static void *_acquire_scoped_lock_no_op(struct c_utils_scoped_lock *s_lock, struct c_utils_scoped_lock_log_info info) {
   return NULL + 1;
}

static void _misc_scoped_lock_no_op(struct c_utils_scoped_lock *s_lock) { }

struct c_utils_scoped_lock *c_utils_scoped_lock_no_op() {
   struct c_utils_scoped_lock *s_lock;
   C_UTILS_ON_BAD_CALLOC(s_lock, NULL, sizeof(*s_lock))
      return NULL;

   s_lock->acquire0 = s_lock->acquire1 = _acquire_scoped_lock_no_op;
   s_lock->dispose = s_lock->release = _misc_scoped_lock_no_op;

   return s_lock;
}

void c_utils_scoped_lock_destroy(struct c_utils_scoped_lock *lock) {
   lock->dispose(lock);
   free(lock);
}

/*
   Mutex definitions
*/

static void _destroy_scoped_lock_mutex(struct c_utils_scoped_lock *s_lock) {
   // TODO: Make use of events so if the mutex is already locked, we wait until it is finished to destroy
   int errcode = pthread_mutex_destroy(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_ERROR(s_lock->logger, "pthread_mutex_destroy: \"%s\"", strerror(errcode));
   

   free(s_lock->lock);
}

static void *_acquire_scoped_lock_mutex(struct c_utils_scoped_lock *s_lock, struct c_utils_scoped_lock_log_info info) {
   int errcode = pthread_mutex_lock(s_lock->lock);
   if (errcode) {
      C_UTILS_LOG_LOCK_FAILURE(s_lock->logger, info, "pthread_mutex_lock: \"%s\"", strerror(errcode));
      exit(EXIT_FAILURE);
   }
   else {
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Obtained lock...");
      s_lock->log_info = info;
   }
   

   return s_lock;
}

static void _release_scoped_lock_mutex(struct c_utils_scoped_lock *s_lock) {
   C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Releasing lock...");

   int errcode = pthread_mutex_unlock(s_lock->lock);
   if (errcode)
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_mutex_unlock: \"%s\"", strerror(errcode));
}

struct c_utils_scoped_lock *c_utils_scoped_lock_mutex(pthread_mutexattr_t *attr, struct c_utils_logger *logger) {
   pthread_mutex_t *lock;
   C_UTILS_ON_BAD_MALLOC(lock, logger, sizeof(*lock))
      goto err;

   int failure = pthread_mutex_init(lock, attr);
   if(failure) {
      C_UTILS_LOG_ERROR(logger, "pthread_mutex_init: \"%s\"", strerror(failure));
      goto err_lock_init;
   }

   return c_utils_scoped_lock_mutex_from(lock, logger);

   err_lock_init:
      free(lock);
   err:
      return NULL;
}

struct c_utils_scoped_lock *c_utils_scoped_lock_mutex_from(pthread_mutex_t *lock, struct c_utils_logger *logger) {
   if(!lock) {
      C_UTILS_LOG_INFO(logger, "Passed lock was NULL, all locking operations set to a NO-OP!");
      return c_utils_scoped_lock_no_op();
   }

   struct c_utils_scoped_lock *s_lock;
   C_UTILS_ON_BAD_CALLOC(s_lock, logger, sizeof(*s_lock))
      return NULL;

   s_lock->lock = lock;
   s_lock->dispose = _destroy_scoped_lock_mutex;
   s_lock->acquire0 = _acquire_scoped_lock_mutex;
   s_lock->acquire1 = _bad_acquire1;
   s_lock->release = _release_scoped_lock_mutex;
   s_lock->logger = logger;

   return s_lock;
}

/*
   SpinLock Definitions
*/


static void _destroy_scoped_lock_spinlock(struct c_utils_scoped_lock *s_lock) {
   // TODO: Make use of events so if the mutex is already locked, we wait until it is finished to destroy
   int errcode = pthread_spin_destroy(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_spin_destroy: \"%s\"", strerror(errcode));
   
   free(s_lock->lock);
}

static void *_acquire_scoped_lock_spinlock(struct c_utils_scoped_lock *s_lock, struct c_utils_scoped_lock_log_info info) {
   int errcode = pthread_spin_lock(s_lock->lock);
   if (errcode) {
      C_UTILS_LOG_LOCK_FAILURE(s_lock->logger, info, "pthread_spin_lock: \"%s\"", strerror(errcode));
      exit(EXIT_FAILURE);
   } else {
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Obtained lock...");
      s_lock->log_info = info;
   }

   return s_lock;
}

static void _release_scoped_lock_spinlock(struct c_utils_scoped_lock *s_lock) {
   C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Releasing lock...");

   int errcode = pthread_spin_unlock(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_spin_unlock: \"%s\"", strerror(errcode));
}

struct c_utils_scoped_lock *c_utils_scoped_lock_spinlock(int flags, struct c_utils_logger *logger) {
   pthread_spinlock_t *lock;
   C_UTILS_ON_BAD_CALLOC(lock, logger, sizeof(*lock))
      goto err;

   int failure = pthread_spin_init(lock, flags);
   if(failure) {
      C_UTILS_LOG_ERROR(logger, "pthread_spin_init: \"%s\"", strerror(failure));
      goto err_lock_init;
   }

   return c_utils_scoped_lock_spinlock_from(lock, logger);

   err_lock_init:
      free(lock);
   err:
      return NULL;

}

struct c_utils_scoped_lock *c_utils_scoped_lock_spinlock_from(pthread_spinlock_t *lock, struct c_utils_logger *logger) {
   if(!lock) {
      C_UTILS_LOG_INFO(logger, "Passed lock was NULL, all locking operations set to a NO-OP!");
      return c_utils_scoped_lock_no_op();
   }

   struct c_utils_scoped_lock *s_lock;
   C_UTILS_ON_BAD_CALLOC(s_lock, logger, sizeof(*s_lock))
      return NULL;

   s_lock->lock = lock;
   s_lock->dispose = _destroy_scoped_lock_spinlock;
   s_lock->acquire0 = _acquire_scoped_lock_spinlock;
   s_lock->acquire1 = _bad_acquire1;
   s_lock->release = _release_scoped_lock_spinlock;
   s_lock->logger = logger;

   return s_lock;
}

/*
   RWLock
*/

static void _destroy_scoped_lock_rwlock(struct c_utils_scoped_lock *s_lock) {
   int errcode = pthread_rwlock_destroy(s_lock->lock);
   if (errcode)
      C_UTILS_LOG_ERROR(s_lock->logger, "pthread_rwlock_destroy: \"%s\"", strerror(errcode));

   free(s_lock->lock);
}

static void *_acquire_scoped_lock_rwlock_write(struct c_utils_scoped_lock *s_lock, struct c_utils_scoped_lock_log_info info) {
   int errcode = pthread_rwlock_wrlock(s_lock->lock);
   if (errcode) {
      C_UTILS_LOG_LOCK_FAILURE(s_lock->logger, info, "pthread_rwlock_wrlock: \"%s\"", strerror(errcode));
      exit(EXIT_FAILURE);
   } else {
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Obtained lock...");
      s_lock->log_info = info;
   }

   return s_lock;
}

/*
   BUG: Unlike wrlock, mutex, spinlock, etc. Readlock can be acquired by more than one thread, hence this
   means of updating the log_info will not suffice. Think of some other way to do so.
*/
static void *_acquire_scoped_lock_rwlock_read(struct c_utils_scoped_lock *s_lock, struct c_utils_scoped_lock_log_info info) {
   int errcode = pthread_rwlock_rdlock(s_lock->lock);
   if (errcode) {
      C_UTILS_LOG_LOCK_FAILURE(s_lock->logger, info, "pthread_rwlock_rdlock: \"%s\"", strerror(errcode));
      exit(EXIT_FAILURE);
   } else {
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Obtained lock...");
      s_lock->log_info = info;
   }

   return s_lock;
}

static void _release_scoped_lock_rwlock(struct c_utils_scoped_lock *s_lock) {
   C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_TRACE, "Releasing lock...");

   int errcode = pthread_rwlock_unlock(s_lock->lock);
   if (errcode)  
      C_UTILS_LOG_LOCK(s_lock, C_UTILS_LOG_LEVEL_ERROR, "pthread_rwlock_unlock: \"%s\"", strerror(errcode));
}

struct c_utils_scoped_lock *c_utils_scoped_lock_rwlock(pthread_rwlockattr_t *attr, struct c_utils_logger *logger) {
   pthread_rwlock_t *lock;
   C_UTILS_ON_BAD_MALLOC(lock, logger, sizeof(*lock))
      goto err;
   
   int failure = pthread_rwlock_init(lock, attr);
   if (failure) {
      C_UTILS_LOG_ERROR(logger, "pthread_spin_init: \"%s\"", strerror(failure));
      goto err_lock_init;
   }
   
   return c_utils_scoped_lock_rwlock_from(lock, logger);

   err_lock_init:
      free(lock);
   err:
      return NULL;
}

struct c_utils_scoped_lock *c_utils_scoped_lock_rwlock_from(pthread_rwlock_t *lock, struct c_utils_logger *logger) {
   if(!lock) {
      C_UTILS_LOG_INFO(logger, "Passed lock was NULL, all locking operations set to a NO-OP!");
      return c_utils_scoped_lock_no_op();
   }

   struct c_utils_scoped_lock *s_lock;
   C_UTILS_ON_BAD_CALLOC(s_lock, logger, sizeof(*s_lock))
      return NULL;

   s_lock->lock = lock;
   s_lock->dispose = _destroy_scoped_lock_rwlock;
   s_lock->acquire0 = _acquire_scoped_lock_rwlock_write;
   s_lock->acquire1 = _acquire_scoped_lock_rwlock_read;
   s_lock->release = _release_scoped_lock_rwlock;
   s_lock->logger = logger;

   return s_lock;
}