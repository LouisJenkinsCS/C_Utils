#ifndef MU_EVENTS_H
#define MU_EVENTS_H

#include <MU_Logger.h>
#include <MU_Flags.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>


/// When the signal times out, signal and broadcast the event.
#define MU_EVENT_SIGNAL_ON_TIMEOUT 1 << 0

/// When the event is created, it is automatically flagged as signaled.
#define MU_EVENT_SIGNALED_BY_DEFAULT 1 << 1

/// When the last thread leaves the event, it will flag the event as being non-signaled.
#define MU_EVENT_AUTO_RESET 1 << 2

#define MU_EVENT_MAX_LEN 64

typedef struct {
	/// The lock associated with an event's condition variable.
	pthread_mutex_t *event_lock;
	/// The condition variable used to wait on an event.
	pthread_cond_t *event_signal;
	/// Bitmask flags for certain conditions.
	unsigned int flags;
	/// An atomic flag signaling whether or not an event is set to be waited on or not.
	atomic_bool signaled;
	/// The amount of threads waiting on this event.
	atomic_int waiting_threads;
	/// The name of the event.
	char name[MU_EVENT_MAX_LEN + 1];
	/// The logger used to log events.
	MU_Logger_t *logger;
} MU_Event_t;

MU_Event_t *MU_Event_create(const char *event_name, MU_Logger_t *logger, unsigned int flags);

bool MU_Event_reset(MU_Event_t *event, unsigned int thread_id);

bool MU_Event_wait(MU_Event_t *event, long long int timeout, unsigned int thread_id);

bool MU_Event_signal(MU_Event_t *event, unsigned int thread_id);

bool MU_Event_destroy(MU_Event_t *event, unsigned int thread_id);

#endif /* endif MU_EVENTS_H */