#include <MU_Logger.h>
#include <pthread.h>

/*
* MU_Cond_Locks, or Misc Util's Conditional Locks, is a wrapper library for the pthread library, or POSIX threads,
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

int MU_Cond_rwlock_init(pthread_rwlock_t *lock, pthread_rwlockattr_t *attr, MU_Logger_t *logger);

int MU_Cond_rwlock_wrlock(pthread_rwlock_t *lock, MU_Logger_t *logger);

int MU_Cond_rwlock_rdlock(pthread_rwlock_t *lock, MU_Logger_t *logger);

int MU_Cond_rwlock_unlock(pthread_rwlock_t *lock, MU_Logger_t *logger);

int MU_Cond_rwlock_destroy(pthread_rwlock_t *lock, MU_Logger_t *logger);

int MU_Cond_mutex_init(pthread_mutex_t *lock, pthread_mutexattr_t *attr, MU_Logger_t *logger);

int MU_Cond_mutex_lock(pthread_mutex_t *lock, MU_Logger_t *logger);

int MU_Cond_mutex_unlock(pthread_mutex_t *lock, MU_Logger_t *logger);

int MU_Cond_mutex_destroy(pthread_mutex_t *lock, MU_Logger_t *logger);