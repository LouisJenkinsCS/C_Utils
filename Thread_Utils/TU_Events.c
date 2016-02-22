#include <TU_Events.h>

static const char *format = "Thread #%u: '%s'";

static void auto_reset_handler(TU_Event_t *event, unsigned int thread_id){
	if(TU_FLAG_GET(event->flags, TU_EVENT_AUTO_RESET_ON_LAST)){
		// Last one out hits the lights. (If we are the last thread, reset event!)
		if(atomic_load(&event->waiting_threads) == 1){
			atomic_store(&event->signaled, false);
			TU_LOG_EVENT(event->logger, event->name, format, thread_id, "Auto-Reset Event Signal As Last Thread...");
		}
	} else if(TU_FLAG_GET(event->flags, TU_EVENT_AUTO_RESET)){
		// Difference between this at ON_LAST is that this will reset the event when any thread leaves.
		atomic_store(&event->signaled, false);
		TU_LOG_EVENT(event->logger, event->name, format, thread_id, "Auto-Reset Event Signal...");
	}
}

TU_Event_t *TU_Event_create(const char *event_name, MU_Logger_t *logger, unsigned int flags){
	bool lock_initialized = false, cond_initialized = false;
	TU_Event_t *event = calloc(1, sizeof(TU_Event_t));
	if(!event){
		TU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	event->event_lock = malloc(sizeof(pthread_mutex_t));
	if(!event->event_lock){
		TU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	int init_failed = pthread_mutex_init(event->event_lock, NULL);
	if(init_failed){
		TU_LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(errno));
		goto error;
	}
	lock_initialized = true;
	event->event_signal = malloc(sizeof(pthread_cond_t));
	if(!event->event_signal){
		TU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	init_failed = pthread_cond_init(event->event_signal, NULL);
	if(init_failed){
		TU_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(init_failed));
	}
	cond_initialized = true;
	event->signaled = ATOMIC_VAR_INIT(TU_FLAG_GET(flags, TU_EVENT_SIGNALED_BY_DEFAULT));
	event->waiting_threads = ATOMIC_VAR_INIT(0);
	if(event_name){
		snprintf(event->name, TU_EVENT_MAX_LEN, "%s", event_name);
	}
	event->logger = logger;
	event->flags = flags;
	TU_LOG_EVENT(event->logger, event->name, "Event created!");
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

bool TU_Event_reset(TU_Event_t *event, unsigned int thread_id){
	if(!event) return false;
	if(atomic_load(&event->signaled) == true){
		atomic_store(&event->signaled, false);
		TU_LOG_EVENT(event->logger, event->name, format, thread_id,  "Reset Event Flag...");
	}
	return true;
}

bool TU_Event_wait(TU_Event_t *event, long long int timeout, unsigned int thread_id){
	if(!event) return false;
	atomic_fetch_add(&event->waiting_threads, 1);
	if(atomic_load(&event->signaled) == true){
		auto_reset_handler(event, thread_id);
		atomic_fetch_sub(&event->waiting_threads, 1);
		return true;
	}
	TU_LOG_EVENT(event->logger, event->name, format, thread_id, "Waiting For Event Signal...");
	pthread_mutex_lock(event->event_lock);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;
	while(atomic_load(&event->signaled) == false){
		int errcode;
		if(timeout < 0){
			errcode = pthread_cond_wait(event->event_signal, event->event_lock);
		} else {
			errcode = pthread_cond_timedwait(event->event_signal, event->event_lock, &ts);
		}
		if(errcode){
			bool retval = false;
			if(errcode != ETIMEDOUT){
				TU_LOG_ERROR(event->logger, "pthread_cond_wait: '%s'", strerror(errcode));
			} else {
				if(TU_FLAG_GET(event->flags, TU_EVENT_SIGNAL_ON_TIMEOUT)){
					atomic_store(&event->signaled, true);
					pthread_cond_broadcast(event->event_signal);
					retval = true;
					TU_LOG_EVENT(event->logger, event->name, format, thread_id, "Timed Out, Signaling Other Threads...");
				}
			}
			pthread_mutex_unlock(event->event_lock);
			atomic_fetch_sub(&event->waiting_threads, 1);
			return retval;
		}
	}
	TU_LOG_EVENT(event->logger, event->name, format, thread_id, "Received Event Signal...");
	auto_reset_handler(event, thread_id);
	pthread_mutex_unlock(event->event_lock);
	atomic_fetch_sub(&event->waiting_threads, 1);
	return true;
}

bool TU_Event_signal(TU_Event_t *event, unsigned int thread_id){
	if(!event) return false;
	if(atomic_load(&event->signaled) == true){
		return true;
	}
	pthread_mutex_lock(event->event_lock);
	atomic_store(&event->signaled, true);
	pthread_cond_broadcast(event->event_signal);
	pthread_mutex_unlock(event->event_lock);
	TU_LOG_EVENT(event->logger, event->name, format, thread_id, "Event Signaled...");
	return true;
}

bool TU_Event_destroy(TU_Event_t *event, unsigned int thread_id){
	if(!event) return false;
	TU_LOG_EVENT(event->logger, event->name, "Broadcasting Event Signal...");
	TU_Event_signal(event, thread_id);
	while(atomic_load(&event->waiting_threads) > 0){
		pthread_yield();
	}
	pthread_mutex_destroy(event->event_lock);
	pthread_cond_destroy(event->event_signal);
	free(event->event_lock);
	free(event->event_signal);
	TU_LOG_EVENT(event->logger, event->name, "Destroying Event...");
	free(event);
	return true;
}