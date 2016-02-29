#include "../io/logger.h"
#include <pthread.h>

/*
* Cond_Locks is a wrapper library for the pthread library, or POSIX threads,
* which allow you to conditionally operate on a lock; meaning, you can safely pass uninitialized (NULL) locks safely
* without a segmentation fault or any other such complications, as well as have any errors logged for you if you pass
* a logger to it. For example, lets say you have a very simple program, say a data structure, and you want to allow the user
* to choose whether or not they want a thread-safe one with locking, or one without it. Normally, this would require one of the
* following:
*
* A) Extensive and repetitive checks for whether or not the locks exist, or...
* B) Creating a separate implementation for a single-threaded one. Of course there's...
* C) Create a lockless implementation.
*
* A is too repetitive and B is unneccesary, although C may be the optimial choice, it may not be possible for all
* data structures. The below functions also take a logger, which can optionally be NULL if you don't want to worry about
* logging any errors, but if you do, you no longer need to worry about handling printing errno any longer, and any errcodes are
* returned directly to be handled by the user. 
*
* This wrapper doesn't claim to do anything special, in fact it's only used for my own convenience, and hopefully to that of others.
*/

/**
 * Conditional wrapper for pthread_rwlock_init, which logs any errors, and only operates on the lock if it is not NULL.
 *
 * @param lock The lock to be operated on if it is not NULL.
 * @param attr The attribute to be used to initialize the lock.
 * @param storage The variable to store the success or failure of rwlock init.
 * @param logger The logger used to log any errors.
 * @return 0 on success, a negative number resembling it's error code on failure.
 */
#define COND_RWLOCK_INIT(lock, attr, storage, logger) do { \
	if (lock) { \
	    int errcode = pthread_rwlock_init(lock, attr); \
	    if (errcode) { \
	        C_UTILS_LOG_ERROR(logger, "pthread_rwlock_init: '%s'", strerror(errcode)); \
	    } \
	    storage = errcode ? 0 : 1; \
   	} \
} while (0) 

#define COND_RWLOCK_WRLOCK(lock, logger) do { \
	if (lock) { \
      int errcode = pthread_rwlock_wrlock(lock); \
      if (errcode) { \
         C_UTILS_LOG_ERROR(logger, "pthread_rwlock_wrlock: '%s'", strerror(errcode)); \
      } \
   } \
} while (0)

#define COND_RWLOCK_RDLOCK(lock, logger) do { \
	if (lock) { \
      int errcode = pthread_rwlock_rdlock(lock); \
      if (errcode) { \
         C_UTILS_LOG_ERROR(logger, "pthread_rwlock_rdlock: '%s'", strerror(errcode)); \
      } \
   } \
} while (0)

#define COND_RWLOCK_UNLOCK(lock, logger) do { \
	if (lock) { \
      int errcode = pthread_rwlock_unlock(lock); \
      if (errcode) { \
         C_UTILS_LOG_ERROR(logger, "pthread_rwlock_unlock: '%s'", strerror(errcode)); \
      } \
   } \
} while (0)

#define COND_RWLOCK_DESTROY(lock, logger) do { \
	if (lock) { \
      int errcode = pthread_rwlock_destroy(lock); \
      if (errcode) { \
         C_UTILS_LOG_ERROR(logger, "pthread_rwlock_destroy: '%s'", strerror(errcode)); \
      } \
      free(lock); \
   } \
} while (0)

#define COND_MUTEX_INIT(lock, attr, storage, logger) do { \
	if (lock) { \
      int errcode = pthread_mutex_init(lock, attr); \
      if (errcode) { \
         C_UTILS_LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(errcode)); \
      } \
      storage = errcode; \
   } \
} while (0)

#define COND_MUTEX_LOCK(lock, logger) do { \
	if (lock) { \
      int errcode = pthread_mutex_lock(lock); \
      if (errcode) { \
         C_UTILS_LOG_ERROR(logger, "pthread_rwlock_lock: '%s'", strerror(errcode)); \
      } \
   } \
} while (0)

#define COND_MUTEX_UNLOCK(lock, logger) do { \
	if (lock) { \
      int errcode = pthread_mutex_unlock(lock); \
      if (errcode) { \
         C_UTILS_LOG_ERROR(logger, "pthread_rwlock_unlock: '%s'", strerror(errcode)); \
      } \
   } \
} while (0)

#define COND_MUTEX_DESTROY(lock, logger) do { \
	if (lock) { \
      int errcode = pthread_mutex_destroy(lock); \
      if (errcode) { \
         C_UTILS_LOG_ERROR(logger, "pthread_rwlock_destroy: '%s'", strerror(errcode)); \
      } \
   } \
} while (0)
