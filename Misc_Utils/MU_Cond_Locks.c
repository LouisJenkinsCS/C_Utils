#include <MU_Cond_Locks.h>


int MU_Cond_rwlock_init(pthread_rwlock_t *lock, pthread_rwlockattr_t *attr, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_rwlock_init(lock, attr);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_rwlock_init->pthread_rwlock_init: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}

int MU_Cond_rwlock_wrlock(pthread_rwlock_t *lock, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_rwlock_wrlock(lock);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_rwlock_wrlock->pthread_rwlock_wrlock: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}

int MU_Cond_rwlock_rdlock(pthread_rwlock_t *lock, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_rwlock_rdlock(lock);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_rwlock_rdlock->pthread_rwlock_rdlock: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}

int MU_Cond_rwlock_unlock(pthread_rwlock_t *lock, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_rwlock_unlock(lock);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_rwlock_unlock->pthread_rwlock_unlock: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}

int MU_Cond_rwlock_destroy(pthread_rwlock_t *lock, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_rwlock_destroy(lock);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_rwlock_destroy->pthread_rwlock_destroy: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}

int MU_Cond_mutex_init(pthread_mutex_t *lock, pthread_mutexattr_t *attr, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_mutex_init(lock, attr);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_mutex_init->pthread_mutex_init: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}

int MU_Cond_mutex_lock(pthread_mutex_t *lock, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_mutex_lock(lock);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_mutex_lock->pthread_rwlock_lock: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}

int MU_Cond_mutex_unlock(pthread_mutex_t *lock, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_mutex_unlock(lock);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_mutex_unlock->pthread_rwlock_unlock: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}

int MU_Cond_mutex_destroy(pthread_mutex_t *lock, MU_Logger_t *logger){
	if(lock){
      int errcode = pthread_mutex_destroy(lock);
      if(errcode){
         MU_LOG_ERROR(logger, "MU_Cond_mutex_destroy->pthread_rwlock_destroy: \"%s\"\n", strerror(errcode));
      }
      return errcode;
   }
   return 0;
}