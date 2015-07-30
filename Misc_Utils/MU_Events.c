#include <MU_Events.h>

MU_Event_t *MU_Event_create(MU_Logger_t *logger){
	bool lock_initialized = false, cond_initialized = false;
	MU_Event_t *event = calloc(1, sizeof(MU_Event_t));
	if(!event){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	event->lock = malloc(sizeof(pthread_mutex_t));
	if(!event->lock){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	int init_failed = pthread_mutex_init(event->lock, NULL);
	if(init_failed){
		MU_LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(errno));
		goto error;
	}
	lock_initialized = true;
	event->cond = malloc(sizeof(pthread_cond_t));
	if(!event->cond){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	init_failed = pthread_cond_init(event->cond, NULL);
	if(init_failed){
		MU_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(init_failed));
	}
	cond_initialized = true;
	event->flag = ATOMIC_VAR_INIT(false);
	event->waiting_threads = ATOMIC_VAR_INIT(0);
	event->logger = logger;
	MU_LOG_VERBOSE(logger, "Event created!");
	return event;

	error:
		if(event){
			if(event->lock){
				if(lock_initialized){
					pthread_mutex_destroy(event->lock);
				}
				free(event->lock);
			}
			if(event->cond){
				if(cond_initialized){
					pthread_cond_destroy(event->cond);
				}
				free(event->cond);
			}
			free(event);
		}
		return NULL;
}

bool MU_Event_set(MU_Event_t *event){
	if(!event) return false;
	atomic_store(&event->flag, true);
	return true;
}

bool MU_Event_wait(MU_Event_t *event, long long int timeout){
	if(!event) return false;
	if(atomic_load(&event->flag) == false){
		return false;
	}
	MU_LOG_VERBOSE(event->logger, "Waiting on event signal!");
	atomic_fetch_add(&event->waiting_threads, 1);
	pthread_mutex_lock(event->lock);
	if(timeout < 0){
		while(atomic_load(&event->flag) == true){
			int errcode = pthread_cond_wait(event->cond, event->lock);
			if(errcode){
				MU_LOG_ERROR(event->logger, "pthread_cond_wait: '%s'", strerror(errcode));
				pthread_mutex_unlock(event->lock);
				atomic_fetch_sub(&event->flag, 1);
				return false;
			}
		}
	} else {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout;
		while(atomic_load(&event->flag) == true){
			int errcode = pthread_cond_timedwait(event->cond, event->lock, &ts);
			if(errcode){
				if(errcode != ETIMEDOUT){
					MU_LOG_ERROR(event->logger, "pthread_cond_wait: '%s'", strerror(errcode));
				}
				pthread_mutex_unlock(event->lock);
				atomic_fetch_sub(&event->flag, 1);
				return false;
			} // End errorcode check
		} // End atomic_load loop.
	} // End timeout condwait
	MU_LOG_VERBOSE(event->logger, "Received event signal!");
	atomic_fetch_sub(&event->flag, 1);
	return true;
} // End function

bool MU_Event_signal(MU_Event_t *event){
	if(!event) return false;
	pthread_mutex_lock(event->lock);
	atomic_store(&event->flag, false);
	pthread_cond_broadcast(event->cond);
	pthread_mutex_unlock(event->lock);
	MU_LOG_VERBOSE(event->logger, "Event signaled!");
	return true;
}

bool MU_Event_destroy(MU_Event_t *event){
	if(!event) return false;
	MU_LOG_VERBOSE(event->logger, "Sending final signal to threads...");
	MU_Event_signal(event);
	while(atomic_load(&event->waiting_threads) > 0){
		pthread_yield();
	}
	pthread_mutex_destroy(event->lock);
	pthread_cond_destroy(event->cond);
	free(event);
	return true;
}