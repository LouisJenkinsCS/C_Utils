#ifndef TU_EVENTS_H
#define TU_EVENTS_H

#include <MU_Logger.h>
#include <MU_Flags.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#ifdef C_UTILS_USE_POSIX_STD
#define event_t TU_Event_t
#define event_create(...) TU_Event_create(__VA_ARGS__)
#define event_reset(...) TU_Event_reset(__VA_ARGS__)
#define event_wait(...) TU_Event_wait(__VA_ARGS__)
#define event_signal(...) TU_Event_signal(__VA_ARGS__)
#define event_destroy(...) TU_Event_destroy(__VA_ARGS__)
#endif

/// When the signal times out, signal and broadcast the event.
#define TU_EVENT_SIGNAL_ON_TIMEOUT 1 << 0

/// When the event is created, it is automatically flagged as signaled.
#define TU_EVENT_SIGNALED_BY_DEFAULT 1 << 1

/// When a thread leaves the event, it will flag the event as being non-signaled.
#define TU_EVENT_AUTO_RESET 1 << 2

/// Like TU_EVENT_AUTO_RESET, execpt it will only flag the event as being non-signaled by the last thread to exit.
#define TU_EVENT_AUTO_RESET_ON_LAST 1 << 3

#define TU_EVENT_MAX_LEN 64

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
	char name[TU_EVENT_MAX_LEN + 1];
	/// The logger used to log events.
	MU_Logger_t *logger;
} TU_Event_t;

/**
 * Creates a new event with the passed name, registers it's logger, and any other interal flags passed.
 * @param event_name Name of this event, which shows up in logger.
 * @param logger Logger to log to.
 * @param flags Internal flags.
 * @return Configured event.
 */
TU_Event_t *TU_Event_create(const char *event_name, MU_Logger_t *logger, unsigned int flags);

/**
 * Resets the event flag to false, meaning it will no longer act as if it had been signaled.
 * @param event Event.
 * @param thread_id Debugging information.
 * @return True or false if event is null.
 */
bool TU_Event_reset(TU_Event_t *event, unsigned int thread_id);

/**
 * Waits on the event unless it is already signaled up to the defined timeout, or indefinite
 * if timeout is < 0.
 * @param event Event.
 * @param timeout Timeout.
 * @param thread_id Debugging Information.
 * @return True if event is not null.
 */
bool TU_Event_wait(TU_Event_t *event, long long int timeout, unsigned int thread_id);

/**
 * Signals to any threads waiting on the event to wake up, and will remain signaled unless
 * AUTO_RESET flag has been passed. 
 * @param event Event.
 * @param thread_id Debugging Information.
 * @return True if event !null.
 */
bool TU_Event_signal(TU_Event_t *event, unsigned int thread_id);

/**
 * Destroys the event, first waking up any threads waiting on this event, and allowing
 * them to gracefully exit.
 * @param event Event.
 * @param thread_id Debugging information.
 * @return If event !null.
 */
bool TU_Event_destroy(TU_Event_t *event, unsigned int thread_id);

#endif /* endif TU_EVENTS_H */