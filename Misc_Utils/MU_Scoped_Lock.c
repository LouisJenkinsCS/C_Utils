#include <MU_Scoped_Lock.h>
#include <MU_Logger.h>
#include <pthread.h>

static MU_Logger_t *logger = NULL;

MU_LOGGER_AUTO_CREATE(logger, "./Logs/MU_Scoped_Lock.log", "w", MU_ALL)

void auto_unlock(MU_Scoped_Lock_t **lock){
	(*lock)->unlock((*lock)->instance);
}

static void destroy_mutex(void *mutex){
	pthread_mutex_destroy(mutex);
	free(mutex);
}

static void lock_mutex(void *mutex){
	pthread_mutex_lock(mutex);
}

static void unlock_mutex(void *mutex){
	pthread_mutex_unlock(mutex);
}

static MU_Scoped_Lock_t *create_scoped_mutex(pthread_mutex_t *lock){
	MU_Scoped_Lock_t *s_lock = calloc(1, sizeof(MU_Scoped_Lock_t));
	if(!s_lock){
		MU_LOG_ASSERT(logger, "calloc: %s", strerror(errno));
		return NULL;
	}
	s_lock->free = destroy_mutex;
	s_lock->lock = lock_mutex;
	s_lock->unlock = unlock_mutex;
}

#define MU_SCOPED_LOCK_CREATE_FROM(lock) _Generic((lock), \
	pthread_mutex_t: create_scoped_mutex, \
	sem_t: created_scoped_semaphore, \
	pthread_spinlock_t: create_scoped_spinlock \
)(lock)