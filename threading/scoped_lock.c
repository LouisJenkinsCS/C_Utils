#include <scoped_lock.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <logger.h>

struct c_utils_scoped_lock
{
   // The instance of lock.
   void *lock;
   // Flag used to make the for loop possible and easy
   volatile bool is_locked;
   // Union used to allow for more than one ways to acquire this lock. I.E RWLock
   // For normal locks, I.E Mutex and Spinlock
   void *(*acquire0)(struct scoped_lock *);
   // For locks with types of locking mechanisms, I.E RWLocks
   void *(*acquire1)(struct scoped_lock *);
   // For when we exit the scope
   void *(*release)(struct scoped_lock *);
   // For when the user frees this, the lock gets freed too.
   void *(*dispose)(struct scoped_lock *);
};

/**
* Called to automatically unlock the passed MU_Scoped_Lock_t instance
* once it leaves the scope. This function gets called by the GCC or
* Clang compiler attribute.
*/
static void _auto_unlock(MU_Scoped_Lock_t **s_lock){
   (*s_lock)->release((*s_lock));
}

static void *_bad_acquire1(MU_Scoped_Lock_t *s_lock){
   MU_ASSERT(s_lock->acquire1, NULL, "The underlying lock does not support a secondary locking mechanism.");
}

/*
   Mutex definitions
*/

static void *_destroy_scoped_lock_mutex(MU_Scoped_Lock_t *s_lock){
   // TODO: Make use of events so if the mutex is already locked, we wait until it is finished to destroy
   pthread_mutex_destroy(s_lock->lock);
   free(s_lock->lock);
   return s_lock;
}

static void *_acquire_scoped_lock_mutex(MU_Scoped_Lock_t *s_lock){
   pthread_mutex_lock(s_lock->lock);
   s_lock->is_locked = true;
   return s_lock;
}

static void *_release_scoped_lock_mutex(MU_Scoped_Lock_t *s_lock){
   s_lock->is_locked = false;
   pthread_mutex_unlock(s_lock->lock);
   return s_lock;
}


c_utils_scoped_lock_t *c_utils_scoped_lock_mutex(pthread_mutex_t *lock){
   MU_Scoped_Lock_t *s_lock = calloc(1, sizeof(MU_Scoped_Lock_t));
   if(!s_lock){
      MU_DEBUG("Calloc: \"%s\"", strerror(errno));
      return NULL;
   }
   s_lock->lock = lock;
   s_lock->dispose = _destroy_scoped_lock_mutex;
   s_lock->acquire0 = _acquire_scoped_lock_mutex;
   s_lock->acquire1 = _bad_acquire1;
   s_lock->release = _release_scoped_lock_mutex;
   s_lock->is_locked = false;
   return s_lock;
}

/*
   SpinLock Definitions
*/


static void *_destroy_scoped_lock_spinlock(MU_Scoped_Lock_t *s_lock){
   // TODO: Make use of events so if the mutex is already locked, we wait until it is finished to destroy
   pthread_spin_destroy(s_lock->lock);
   free(s_lock->lock);
   return s_lock;
}

static void *_acquire_scoped_lock_spinlock(MU_Scoped_Lock_t *s_lock){
   pthread_spin_lock(s_lock->lock);
   s_lock->is_locked = true;
   return s_lock;
}

static void *_release_scoped_lock_spinlock(MU_Scoped_Lock_t *s_lock){
   s_lock->is_locked = false;
   pthread_spin_unlock(s_lock->lock);
   return s_lock;
}

c_utils_scoped_lock_t *c_utils_scoped_lock_spinlock(pthread_spinlock_t *lock){
   MU_Scoped_Lock_t *s_lock = calloc(1, sizeof(MU_Scoped_Lock_t));
   if(!s_lock){
      MU_DEBUG("Calloc: \"%s\"", strerror(errno));
      return NULL;
   }
   s_lock->lock = lock;
   s_lock->dispose = _destroy_scoped_lock_spinlock;
   s_lock->acquire0 = _acquire_scoped_lock_spinlock;
   s_lock->acquire1 = _bad_acquire1;
   s_lock->release = _release_scoped_lock_spinlock;
   s_lock->is_locked = false;
   return s_lock;
}