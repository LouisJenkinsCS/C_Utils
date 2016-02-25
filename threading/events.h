#ifndef TU_EVENTS_H
#define TU_EVENTS_H

#include "io/logger.h"
#include "misc/flags.h"

struct c_utils_event;

#ifdef NO_C_UTILS_PREFIX
typedef struct c_utils_event event_t;
#define event_create(...) c_utils_event_create(__VA_ARGS__)
#define event_reset(...) c_utils_event_reset(__VA_ARGS__)
#define event_wait(...) c_utils_event_wait(__VA_ARGS__)
#define event_signal(...) c_utils_event_signal(__VA_ARGS__)
#define event_destroy(...) c_utils_event_destroy(__VA_ARGS__)
#endif

/// When the signal times out, signal and broadcast the event.
const int EVENT_SIGNAL_ON_TIMEOUT = 1 << 0;

/// When the event is created, it is automatically flagged as signaled.
const int EVENT_SIGNALED_BY_DEFAULT = 1 << 1;

/// When a thread leaves the event, it will flag the event as being non-signaled.
const int EVENT_AUTO_RESET = 1 << 2;

/// Like TU_EVENT_AUTO_RESET, execpt it will only flag the event as being non-signaled by the last thread to exit.
const int EVENT_AUTO_RESET_ON_LAST = 1 << 3;

#define EVENT_MAX_NAME_LEN 64

/**
 * Creates a new event with the passed name, registers it's logger, and any other interal flags passed.
 * @param event_name Name of this event, which shows up in logger.
 * @param logger Logger to log to.
 * @param flags Internal flags.
 * @return Configured event.
 */
struct c_utils_event *c_utils_event_create(const char *event_name, struct c_utils_logger *logger, unsigned int flags);

/**
 * Resets the event flag to false, meaning it will no longer act as if it had been signaled.
 * @param event Event.
 * @param thread_id Debugging information.
 * @return True or false if event is null.
 */
bool c_utils_event_reset(struct c_utils_event *event, unsigned int thread_id);

/**
 * Waits on the event unless it is already signaled up to the defined timeout, or indefinite
 * if timeout is < 0.
 * @param event Event.
 * @param timeout Timeout.
 * @param thread_id Debugging Information.
 * @return True if event is not null.
 */
bool c_utils_event_wait(struct c_utils_event *event, long long int timeout, unsigned int thread_id);

/**
 * Signals to any threads waiting on the event to wake up, and will remain signaled unless
 * AUTO_RESET flag has been passed. 
 * @param event Event.
 * @param thread_id Debugging Information.
 * @return True if event !null.
 */
;

/**
 * Destroys the event, first waking up any threads waiting on this event, and allowing
 * them to gracefully exit.
 * @param event Event.
 * @param thread_id Debugging information.
 * @return If event !null.
 */
bool c_utils_event_destroy(struct c_utils_event *event, unsigned int thread_id);

#endif /* endif TU_EVENTS_H */