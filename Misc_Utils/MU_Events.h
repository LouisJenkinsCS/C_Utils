#ifndef MU_EVENTS_H
#define MU_EVENTS_H

#include <MU_Logger.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

typedef struct {
	/// The lock associated with an event's condition variable.
	pthread_mutex_t *lock;
	/// The condition variable used to wait on an event.
	pthread_cond_t *cond;
	/// An atomic flag signaling whether or not an event is set to be waited on or not.
	atomic_bool flag;
	/// The amount of threads waiting on this event.
	atomic_int waiting_threads;
	/// The logger used to log events.
	MU_Logger_t *logger;
} MU_Event_t;

MU_Event_t *MU_Event_create(MU_Logger_t *logger);

bool MU_Event_set(MU_Event_t *event);

bool MU_Event_wait(MU_Event_t *event, long long int timeout);

bool MU_Event_signal(MU_Event_t *event);

bool MU_Event_destroy(MU_Event_t *event);

#endif /* endif MU_EVENTS_H */