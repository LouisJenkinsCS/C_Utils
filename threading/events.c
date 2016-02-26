#include <threading/events.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

struct c_utils_event {
	/// The lock associated with an event's condition variable.
	pthread_mutex_t *lock;
	/// The condition variable used to wait on an event.
	pthread_cond_t *signal;
	/// Bitmask flags for certain conditions.
	unsigned int flags;
	/// An atomic flag signaling whether or not an event is set to be waited on or not.
	atomic_bool signaled;
	/// The amount of threads waiting on this event.
	atomic_int waiting_threads;
	/// The name of the event.
	char name[EVENT_MAX_NAME_LEN + 1];
	/// The logger used to log events.
	struct c_utils_logger *logger;
};

static const char *format = "Thread #%u: '%s'";

static void auto_reset_handler(c_utils_event *event, unsigned int thread_id){
	if(FLAG_GET(event->flags, EVENT_AUTO_RESET_ON_LAST)){

		// Last one out hits the lights. (If we are the last thread, reset event!)
		if(atomic_load(&event->waiting_threads) == 1){
			atomic_store(&event->signaled, false);
			LOG_EVENT(event->logger, event->name, format, thread_id, "Auto-Reset Event Signal As Last Thread...");
		}
	} else if(FLAG_GET(event->flags, EVENT_AUTO_RESET)){

		// Difference between this at ON_LAST is that this will reset the event when any thread leaves.
		atomic_store(&event->signaled, false);
		LOG_EVENT(event->logger, event->name, format, thread_id, "Auto-Reset Event Signal...");
	}
}

struct c_utils_event *c_utils_event_create(const char *event_name, struct c_utils_logger *logger, unsigned int flags){
	bool lock_initialized = false, cond_initialized = false;
	
	struct c_utils_event *event = calloc(1, sizeof(c_utils_event_t));
	if(!event){
		LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	
	event->lock = malloc(sizeof(pthread_mutex_t));
	if(!event->lock){
		LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	
	int init_failed = pthread_mutex_init(event->lock, NULL);
	if(init_failed){
		LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(errno));
		goto error;
	}
	lock_initialized = true;
	
	event->signal = malloc(sizeof(pthread_cond_t));
	if(!event->signal){
		LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	
	init_failed = pthread_cond_init(event->signal, NULL);
	if(init_failed){
		LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(init_failed));
	}
	cond_initialized = true;
	
	event->signaled = ATOMIC_VAR_INIT(FLAG_GET(flags, EVENT_SIGNALED_BY_DEFAULT));
	event->waiting_threads = ATOMIC_VAR_INIT(0);
	if(event_name){
		snprintf(event->name, EVENT_MAX_NAME_LEN, "%s", event_name);
	}
	event->logger = logger;
	event->flags = flags;
	
	LOG_EVENT(event->logger, event->name, "Event created!");
	
	return event;

	error:
		if(event){
			if(event->lock){
				if(lock_initialized){
					pthread_mutex_destroy(event->lock);
				}
				free(event->lock);
			}
			if(event->signal){
				if(cond_initialized){
					pthread_cond_destroy(event->signal);
				}
				free(event->signal);
			}
			free(event);
		}
		return NULL;
}

bool c_utils_event_reset(struct c_utils_event *event, unsigned int thread_id){
	if(!event) return false;

	if(atomic_load(&event->signaled) == true){
		atomic_store(&event->signaled, false);
		LOG_EVENT(event->logger, event->name, format, thread_id,  "Reset Event Flag...");
	}

	return true;
}

bool c_utils_event_wait(struct c_utils_event *event, long long int timeout, unsigned int thread_id){
	if(!event) return false;

	atomic_fetch_add(&event->waiting_threads, 1);

	if(atomic_load(&event->signaled) == true){
		auto_reset_handler(event, thread_id);
		atomic_fetch_sub(&event->waiting_threads, 1);
		return true;
	}

	LOG_EVENT(event->logger, event->name, format, thread_id, "Waiting For Event Signal...");
	pthread_mutex_lock(event->lock);

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;

	while(atomic_load(&event->signaled) == false){
		int errcode;
		if(timeout < 0){
			errcode = pthread_cond_wait(event->signal, event->lock);
		} else {
			errcode = pthread_cond_timedwait(event->signal, event->lock, &ts);
		}
		
		if(errcode){
			bool retval = false;
			if(errcode != ETIMEDOUT){
				LOG_ERROR(event->logger, "pthread_cond_wait: '%s'", strerror(errcode));
			} else {
				if(FLAG_GET(event->flags, EVENT_SIGNAL_ON_TIMEOUT)){
					atomic_store(&event->signaled, true);
					pthread_cond_broadcast(event->signal);
					retval = true;
					LOG_EVENT(event->logger, event->name, format, thread_id, "Timed Out, Signaling Other Threads...");
				}
			}
			
			pthread_mutex_unlock(event->lock);
			atomic_fetch_sub(&event->waiting_threads, 1);
			return retval;
		}
	}
	
	LOG_EVENT(event->logger, event->name, format, thread_id, "Received Event Signal...");
	auto_reset_handler(event, thread_id);
	pthread_mutex_unlock(event->lock);
	atomic_fetch_sub(&event->waiting_threads, 1);
	
	return true;
}

bool c_utils_event_signal(struct c_utils_event *event, unsigned int thread_id){
	if(!event) return false;
	
	if(atomic_load(&event->signaled) == true){
		return true;
	}
	
	pthread_mutex_lock(event->lock);
	atomic_store(&event->signaled, true);
	pthread_cond_broadcast(event->signal);
	pthread_mutex_unlock(event->lock);
	LOG_EVENT(event->logger, event->name, format, thread_id, "Event Signaled...");
	
	return true;
}

bool c_utils_event_destroy(struct c_utils_event *event, unsigned int thread_id){
	if(!event) return false;
	
	LOG_EVENT(event->logger, event->name, "Broadcasting Event Signal...");
	c_utils_event_signal(event, thread_id);
	while(atomic_load(&event->waiting_threads) > 0){
		pthread_yield();
	}
	
	pthread_mutex_destroy(event->lock);
	pthread_cond_destroy(event->signal);
	free(event->lock);
	free(event->signal);
	LOG_EVENT(event->logger, event->name, "Destroying Event...");
	free(event);
	
	return true;
}