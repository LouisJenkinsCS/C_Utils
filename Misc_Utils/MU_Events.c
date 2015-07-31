#include <MU_Events.h>

/// The common event log format.
static const char *format = "~%s~: '%s'";

MU_Event_t *MU_Event_create(bool default_state, const char *event_name, MU_Logger_t *logger){
	bool lock_initialized = false, cond_initialized = false;
	MU_Event_t *event = calloc(1, sizeof(MU_Event_t));
	if(!event){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	event->event_lock = malloc(sizeof(pthread_mutex_t));
	if(!event->event_lock){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	int init_failed = pthread_mutex_init(event->event_lock, NULL);
	if(init_failed){
		MU_LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(errno));
		goto error;
	}
	lock_initialized = true;
	event->event_signal = malloc(sizeof(pthread_cond_t));
	if(!event->event_signal){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	init_failed = pthread_cond_init(event->event_signal, NULL);
	if(init_failed){
		MU_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(init_failed));
	}
	cond_initialized = true;
	event->signaled = ATOMIC_VAR_INIT(default_state);
	event->waiting_threads = ATOMIC_VAR_INIT(0);
	if(event_name){
		snprintf(event->name, MU_EVENT_MAX_LEN, "%s", event_name);
	}
	event->logger = logger;
	MU_LOG_VERBOSE(logger, format, event_name, "Event created!");
	return event;

	error:
		if(event){
			if(event->event_lock){
				if(lock_initialized){
					pthread_mutex_destroy(event->event_lock);
				}
				free(event->event_lock);
			}
			if(event->event_signal){
				if(cond_initialized){
					pthread_cond_destroy(event->event_signal);
				}
				free(event->event_signal);
			}
			free(event);
		}
		return NULL;
}

bool MU_Event_reset(MU_Event_t *event){
	if(!event) return false;
	atomic_store(&event->signaled, false);
	return true;
}

bool MU_Event_wait(MU_Event_t *event, long long int timeout){
	if(!event) return false;
	atomic_fetch_add(&event->waiting_threads, 1);
	if(atomic_load(&event->signaled) == true){
		atomic_fetch_sub(&event->waiting_threads, 1);
		return true;
	}
	MU_LOG_VERBOSE(event->logger, format, event->name, "Waiting on event signal!");
	pthread_mutex_lock(event->event_lock);
	if(timeout < 0){
		while(atomic_load(&event->signaled) == false){
			int errcode = pthread_cond_wait(event->event_signal, event->event_lock);
			if(errcode){
				MU_LOG_ERROR(event->logger, "pthread_cond_wait: '%s'", strerror(errcode));
				pthread_mutex_unlock(event->event_lock);
				atomic_fetch_sub(&event->waiting_threads, 1);
				return false;
			}
		}
	} else {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += timeout;
		while(atomic_load(&event->signaled) == false){
			int errcode = pthread_cond_timedwait(event->event_signal, event->event_lock, &ts);
			if(errcode){
				if(errcode != ETIMEDOUT){
					MU_LOG_ERROR(event->logger, "pthread_cond_wait: '%s'", strerror(errcode));
				}
				pthread_mutex_unlock(event->event_lock);
				atomic_fetch_sub(&event->waiting_threads, 1);
				return false;
			} // End errorcode check
		} // End atomic_load loop.
	} // End timeout condwait
	MU_LOG_VERBOSE(event->logger, format, event->name, "Received event signal!");
	pthread_mutex_unlock(event->event_lock);
	atomic_fetch_sub(&event->waiting_threads, 1);
	return true;
} // End function

bool MU_Event_signal(MU_Event_t *event){
	if(!event) return false;
	if(atomic_load(&event->signaled) == true){
		return true;
	}
	pthread_mutex_lock(event->event_lock);
	atomic_store(&event->signaled, true);
	pthread_cond_broadcast(event->event_signal);
	pthread_mutex_unlock(event->event_lock);
	MU_LOG_VERBOSE(event->logger, format, event->name, "Event signaled!");
	return true;
}

bool MU_Event_destroy(MU_Event_t *event){
	if(!event) return false;
	MU_LOG_VERBOSE(event->logger, format, event->name, "Sending final signal to threads...");
	MU_Event_signal(event);
	while(atomic_load(&event->waiting_threads) > 0){
		pthread_yield();
	}
	pthread_mutex_destroy(event->event_lock);
	pthread_cond_destroy(event->event_signal);
	free(event);
	return true;
}