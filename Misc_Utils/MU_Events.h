#ifndef MU_EVENTS_H
#define MU_EVENTS_H

#include <MU_Logger.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#define MU_EVENT_MAX_LEN 64

typedef struct {
	/// The lock associated with an event's condition variable.
	pthread_mutex_t *event_lock;
	/// The condition variable used to wait on an event.
	pthread_cond_t *event_signal;
	/// An atomic flag signaling whether or not an event is set to be waited on or not.
	atomic_bool signaled;
	/// The amount of threads waiting on this event.
	atomic_int waiting_threads;
	/// The name of the event.
	char name[MU_EVENT_MAX_LEN + 1];
	/// The logger used to log events.
	MU_Logger_t *logger;
} MU_Event_t;

MU_Event_t *MU_Event_create(bool default_state, const char *event_name, MU_Logger_t *logger);

bool MU_Event_reset(MU_Event_t *event);

bool MU_Event_wait(MU_Event_t *event, long long int timeout);

bool MU_Event_signal(MU_Event_t *event);

bool MU_Event_destroy(MU_Event_t *event);

#endif /* endif MU_EVENTS_H */