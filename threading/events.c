#include "events.h"

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#include "../memory/ref_count.h"

struct c_utils_event {
	/// The lock associated with an event's condition variable.
	pthread_mutex_t lock;
	/// The condition variable used to wait on an event.
	pthread_cond_t signal;
	/// An atomic flag signaling whether or not an event is set to be waited on or not.
	atomic_bool signaled;
	/// The amount of threads waiting on this event.
	atomic_int waiting_threads;
	/// Configuration
	struct c_utils_event_conf conf;
};

static void configure(struct c_utils_event_conf *conf);

static void destroy_event(void *instance);

static void auto_reset_handler(struct c_utils_event *event);



struct c_utils_event *c_utils_event_create() {
	struct c_utils_event_conf conf = {0};
	return c_utils_event_create_conf(&conf);
}

struct c_utils_event *c_utils_event_create_conf(struct c_utils_event_conf *conf) {
	if(!conf)
		return NULL;

	configure(conf);

	struct c_utils_event *event;
	if(conf->flags & C_UTILS_EVENT_RC_INSTANCE) {
		struct c_utils_ref_count_conf rc_conf =
		{
			.logger = conf->logger,
			.destructor = destroy_event
		};

		event = c_utils_ref_create_conf(sizeof(*event), &rc_conf);
	} else {
		event = malloc(sizeof(*event));
	}

	if(!event) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed during creation of event!");
		goto err;
	}
	
	int init_failed = pthread_mutex_init(&event->lock, NULL);
	if (init_failed) {
		C_UTILS_LOG_ERROR(conf->logger, "pthread_mutex_init: '%s'", strerror(errno));
		goto err_lock;
	}
	
	init_failed = pthread_cond_init(&event->signal, NULL);
	if (init_failed) {
		C_UTILS_LOG_ERROR(conf->logger, "pthread_cond_init: '%s'", strerror(init_failed));
		goto err_signal;
	}
		
	event->signaled = ATOMIC_VAR_INIT(conf->flags & C_UTILS_EVENT_SIGNALED_BY_DEFAULT);
	event->waiting_threads = ATOMIC_VAR_INIT(0);
	event->conf = *conf;
	
	C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Created!");
	
	return event;

	err_signal:
		pthread_mutex_destroy(&event->lock);
	err_lock:
		free(event);
	err:
		return NULL;
}

void c_utils_event_reset(struct c_utils_event *event) {
	if (!event) 
		return;

	atomic_store(&event->signaled, true);
	C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Reset!");
}

bool c_utils_event_wait_for(struct c_utils_event *event, long long int timeout) {
	struct timespec tv;

	clock_gettime(CLOCK_MONOTONIC, &tv);
	tv.tv_sec += timeout / 1000;
	tv.tv_nsec += (timeout % 1000) * 1000000L;

	return c_utils_event_wait_until(event, timeout > 0 ? &tv : NULL);
}

bool c_utils_event_wait_until(struct c_utils_event *event, struct timespec *timeout) {
	if(!event)
		return false;

	atomic_fetch_add(&event->waiting_threads, 1);

	if (atomic_load(&event->signaled) == true) {
		auto_reset_handler(event);
		atomic_fetch_sub(&event->waiting_threads, 1);
		return true;
	}

	C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Waiting for signal...");
	pthread_mutex_lock(&event->lock);

	while (atomic_load(&event->signaled) == false) {
		int errcode;
		if (timeout)
			errcode = pthread_cond_timedwait(&event->signal, &event->lock, timeout);
		 else   
			errcode = pthread_cond_wait(&event->signal, &event->lock);
		
		if (errcode) {
			bool retval = false;
			if (errcode != ETIMEDOUT) {
				C_UTILS_LOG_ERROR(event->conf.logger, "pthread_cond_wait: '%s'", strerror(errcode));
			} else if (event->conf.flags & C_UTILS_EVENT_SIGNAL_ON_TIMEOUT) {
					atomic_store(&event->signaled, true);
					pthread_cond_broadcast(&event->signal);
					retval = true;
					C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Timed out, broadcasting to other threads...");
				}
			
			pthread_mutex_unlock(&event->lock);
			atomic_fetch_sub(&event->waiting_threads, 1);
			return retval;
		}
	}

	C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Received signal!");
	
	auto_reset_handler(event);
	pthread_mutex_unlock(&event->lock);
	atomic_fetch_sub(&event->waiting_threads, 1);
	
	return true;
}

void c_utils_event_signal(struct c_utils_event *event) {
	if (!event)
		return;
	
	if (atomic_load(&event->signaled) == true)
		return;
	
	
	pthread_mutex_lock(&event->lock);
	atomic_store(&event->signaled, true);
	pthread_cond_broadcast(&event->signal);
	pthread_mutex_unlock(&event->lock);

	C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Sent signal...");
}

void c_utils_event_destroy(struct c_utils_event *event) {
	if (!event) 
		return;

	if(event->conf.flags & C_UTILS_EVENT_RC_INSTANCE) {
		C_UTILS_REF_DEC(event);
		return;
	}

	destroy_event(event);
}



static void configure(struct c_utils_event_conf *conf) {
	if(!conf->name) {
		if(conf->callbacks.destructors.name)
			conf->callbacks.destructors.name = NULL;

		conf->name = "NONE";
	}
}

static void destroy_event(void *instance) {
	struct c_utils_event *event = instance;
	C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Destroying event, unblocking threads...");
	
	c_utils_event_signal(event);
	
	while (atomic_load(&event->waiting_threads) > 0)
		pthread_yield();
		
	pthread_mutex_destroy(&event->lock);
	pthread_cond_destroy(&event->signal);
	
	C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Finished...");

	free(event);
}

static void auto_reset_handler(struct c_utils_event *event) {
	if (event->conf.flags & C_UTILS_EVENT_AUTO_RESET_ON_LAST) {
		// Last one out hits the lights. (If we are the last thread, reset event!)
		if (atomic_load(&event->waiting_threads) == 1) {
			atomic_store(&event->signaled, false);
			C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Resetting signal as last thread exitted");
		}
	} else if (event->conf.flags & C_UTILS_EVENT_AUTO_RESET) {
		// Difference between this and AUTO_RESET_ON_LAST is that this will reset the event when any thread leaves.
		atomic_store(&event->signaled, false);
		C_UTILS_LOG_EVENT(event->conf.logger, event->conf.name, "Auto-Reset Event Signal...");
	}
}