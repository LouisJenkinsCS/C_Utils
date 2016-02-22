#ifndef MU_SCOPED_LOCK_H
#define MU_SCOPED_LOCK_H

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <MU_Logger.h>

#ifdef C_UTILS_USE_POSIX_STD
#define SCOPED_LOCK(...) MU_SCOPED_LOCK(__VA_ARGS__)
#define SCOPED_LOCK_FROM(...) MU_SCOPED_LOCK_FROM(__VA_ARGS__)
#define scoped_lock_t MU_Scoped_Lock_t
#endif

typedef struct scoped_lock
{
   // The instance of lock.
   void *lock;
   // Flag used to make the for loop possible and easy
   volatile bool is_locked;
   // Union used to allow for more than one ways to acquire this lock. I.E RWLock
   union {
      // For normal locks, I.E Mutex and Spinlock
      void *(*acquire)(struct scoped_lock *);
      // For locks with two kinds of lock acquiring, I.E RWLock
      struct {
         void *(*acquire0)(struct scoped_lock *);
         void *(*acquire1)(struct scoped_lock *);
      };
   };
   // For when we exit the scope
   void *(*release)(struct scoped_lock *);
   // For when the user frees this, the lock gets freed too.
   void *(*dispose)(struct scoped_lock *);
} MU_Scoped_Lock_t;

/**
* Called to automatically unlock the passed MU_Scoped_Lock_t instance
* once it leaves the scope. This function gets called by the GCC or
* Clang compiler attribute.
*/
void _auto_unlock(MU_Scoped_Lock_t **s_lock){
   (*s_lock)->release((*s_lock));
}

/*
   Mutex definitions
*/

void *_destroy_scoped_lock_mutex(MU_Scoped_Lock_t *s_lock){
   // TODO: Make use of events so if the mutex is already locked, we wait until it is finished to destroy
   pthread_mutex_destroy(s_lock->lock);
   free(s_lock->lock);
   return s_lock;
}

void *_acquire_scoped_lock_mutex(MU_Scoped_Lock_t *s_lock){
   pthread_mutex_lock(s_lock->lock);
   s_lock->is_locked = true;
   return s_lock;
}

void *_release_scoped_lock_mutex(MU_Scoped_Lock_t *s_lock){
   s_lock->is_locked = false;
   pthread_mutex_unlock(s_lock->lock);
   return s_lock;
}

MU_Scoped_Lock_t *_scoped_lock_mutex(pthread_mutex_t *lock){
   MU_Scoped_Lock_t *s_lock = calloc(1, sizeof(MU_Scoped_Lock_t));
   if(!s_lock){
      MU_DEBUG("Calloc: \"%s\"", strerror(errno));
      return NULL;
   }
   s_lock->lock = lock;
   s_lock->dispose = _destroy_scoped_lock_mutex;
   s_lock->acquire = _acquire_scoped_lock_mutex;
   s_lock->release = _release_scoped_lock_mutex;
   s_lock->is_locked = false;
   return s_lock;
}

/*
   SpinLock Definitions
*/

void *_destroy_scoped_lock_spinlock(MU_Scoped_Lock_t *s_lock){
   // TODO: Make use of events so if the mutex is already locked, we wait until it is finished to destroy
   pthread_spin_destroy(s_lock->lock);
   free(s_lock->lock);
   return s_lock;
}

void *_acquire_scoped_lock_spinlock(MU_Scoped_Lock_t *s_lock){
   pthread_spin_lock(s_lock->lock);
   s_lock->is_locked = true;
   return s_lock;
}

void *_release_scoped_lock_spinlock(MU_Scoped_Lock_t *s_lock){
   s_lock->is_locked = false;
   pthread_spin_unlock(s_lock->lock);
   return s_lock;
}

MU_Scoped_Lock_t *_scoped_lock_spinlock(pthread_spinlock_t *lock){
   MU_Scoped_Lock_t *s_lock = calloc(1, sizeof(MU_Scoped_Lock_t));
   if(!s_lock){
      MU_DEBUG("Calloc: \"%s\"", strerror(errno));
      return NULL;
   }
   s_lock->lock = lock;
   s_lock->dispose = _destroy_scoped_lock_spinlock;
   s_lock->acquire = _acquire_scoped_lock_spinlock;
   s_lock->release = _release_scoped_lock_spinlock;
   s_lock->is_locked = false;
   return s_lock;
}


#define MU_SCOPED_LOCK_FROM(lock) _Generic((lock), pthread_mutex_t *: _scoped_lock_mutex, pthread_spinlock_t *: _scoped_lock_spinlock)(lock)

/*
   Note how we create a temporary variable to point to the
   passed lock. This is because it needs to be defined
   inside of the scope in order to have the cleanup function be
   called.
*/ 
#define MU_SCOPED_LOCK(s_lock) for(MU_Scoped_Lock_t *tmp_lock __attribute__ ((__cleanup__(_auto_unlock))) = s_lock, *_test = tmp_lock->acquire(tmp_lock); _test; _test = NULL)

#endif /* MU_SCOPED_LOCK_H */