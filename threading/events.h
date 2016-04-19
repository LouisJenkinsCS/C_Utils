#ifndef C_UTILS_EVENTS_H
#define C_UTILS_EVENTS_H

#include "../io/logger.h"
#include "../misc/flags.h"

#define C_UTILS_EVENT_MAX_NAME_LEN 64

/*
	Simple implementation for Win32 events using conditional variables and mutexes. Allows you to 
	wait on certain events, signal events, reset them, etc. Also is configurable by passing certain flags.
*/
struct c_utils_event;

struct c_utils_event_conf {
	int flags;
	struct c_utils_logger *logger;
	const char *name;
	struct {
		struct {
			void (*name)(void *);
		} destructors;
	} callbacks;
};

/// When the signal times out, signal and broadcast the event.
#define C_UTILS_EVENT_SIGNAL_ON_TIMEOUT 1 << 0

/// When the event is created, it is automatically flagged as signaled.
#define C_UTILS_EVENT_SIGNALED_BY_DEFAULT 1 << 1

/// When a thread leaves the event, it will flag the event as being non-signaled.
#define C_UTILS_EVENT_AUTO_RESET 1 << 2

/// Like EVENT_AUTO_RESET, execpt it will only flag the event as being non-signaled by the last thread to exit.
#define C_UTILS_EVENT_AUTO_RESET_ON_LAST 1 << 3

#define C_UTILS_EVENT_RC_INSTANCE 1 << 4

#ifdef NO_C_UTILS_PREFIX
/*
	Typedef
*/
typedef struct c_utils_event event_t;
typedef struct c_utils_event_conf event_conf_t;

/*
	Macros
*/
#define EVENT_SIGNAL_ON_TIMEOUT C_UTILS_EVENT_SIGNAL_ON_TIMEOUT
#define EVENT_SIGNALED_BY_DEFAULT C_UTILS_EVENT_SIGNALED_BY_DEFAULT
#define EVENT_AUTO_RESET C_UTILS_EVENT_AUTO_RESET
#define EVENT_AUTO_RESET_ON_LAST C_UTILS_EVENT_AUTO_RESET_ON_LAST
#define EVENT_RC_INSTANCE C_UTILS_EVENT_RC_INSTANCE

/*
	Functions
*/
#define event_create(...) c_utils_event_create(__VA_ARGS__)
#define event_reset(...) c_utils_event_reset(__VA_ARGS__)
#define event_wait(...) c_utils_event_wait(__VA_ARGS__)
#define event_signal(...) c_utils_event_signal(__VA_ARGS__)
#define event_destroy(...) c_utils_event_destroy(__VA_ARGS__)
#endif

/**
 * Creates a new event with the passed name, registers it's logger, and any other interal flags passed.
 * @param event_name Name of this event, which shows up in logger.
 * @param logger Logger to log to.
 * @param flags Internal flags.
 * @return Configured event.
 */
struct c_utils_event *c_utils_event_create();

struct c_utils_event *c_utils_event_create_conf(struct c_utils_event_conf *conf);

/**
 * Resets the event flag to false, meaning it will no longer act as if it had been signaled.
 * @param event Event.
 * @param thread_id Debugging information.
 * @return True or false if event is null.
 */
void c_utils_event_reset(struct c_utils_event *event);

/**
 * Waits on the event unless it is already signaled up to the defined timeout, or indefinite
 * if timeout is < 0.
 * @param event Event.
 * @param timeout Timeout.
 * @param thread_id Debugging Information.
 * @return True if event is not null.
 */
bool c_utils_event_wait_for(struct c_utils_event *event, long long int timeout);

bool c_utils_event_wait_until(struct c_utils_event *event, struct timespec *timeout);

/**
 * Signals to any threads waiting on the event to wake up, and will remain signaled unless
 * AUTO_RESET flag has been passed. 
 * @param event Event.
 * @param thread_id Debugging Information.
 * @return True if event !null.
 */
void c_utils_event_signal(struct c_utils_event *event);

/**
 * Destroys the event, first waking up any threads waiting on this event, and allowing
 * them to gracefully exit.
 * @param event Event.
 * @param thread_id Debugging information.
 * @return If event !null.
 */
void c_utils_event_destroy(struct c_utils_event *event);

#endif /* endif C_UTILS_EVENTS_H */