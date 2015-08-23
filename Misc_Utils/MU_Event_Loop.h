#ifndef MU_EVENT_LOOP_H
#define MU_EVENT_LOOP_H

#include <stdbool.h>
#include <sys/time.h>
#include <Linked_List.h>
#include <MU_Events.h>

typedef void *(*MU_Event_Prepare)();
typedef bool (*MU_Event_Check)(void *);
typedef bool (*MU_Event_Dispatch)(void *);
typedef bool (*MU_Event_Finalize)(void *);

typedef struct {
	/// Maintains list of sources to check.
	Linked_List_t *sources;
	/// Keep-Alive flag
	_Atomic bool keep_alive;
	/// The event to wait on for it to finish.
	MU_Event_t *finished;
} MU_Event_Loop_t;

/*
	MU_Event_Prepare -> Data; Prepares the event
	MU_Event_Dispatch -> Data; Dispatches the data if it is ready.
	MU_Event_Finalize -> Data; Destroys the data if it is finished.
*/
typedef struct {
	/// Data returned from an event.
	void *data;
	/// Callback used to prepare the data passed to it.
	MU_Event_Prepare prepare;
	/// Callback used to check if event is ready.
	MU_Event_Check check;
	/// Callback used to dispatch said event.
	MU_Event_Dispatch dispatch;
	/// Callback used whenever the main loop finishes.
	MU_Event_Finalize finalize;
	/// Internal flags used to help maintain information about the event source.
	unsigned int flags;
	/// The timeval used to record the absolute time of the next timeout.
	struct timeval next_timeout;
	/// The timeout the timeval will be set to after triggering.
	unsigned int timeout;
} MU_Event_Source_t;

MU_Event_Source_t *MU_Event_Source_create(MU_Event_Prepare prepare_cb, MU_Event_Check check_cb, MU_Event_Dispatch dispatch_cb, MU_Event_Finalize finalize_cb, unsigned long long int timeout);

bool MU_Event_Source_destroy(MU_Event_Source_t *source);

MU_Event_Loop_t *MU_Event_Loop_create(void);

bool MU_Event_Loop_add(MU_Event_Loop_t *loop, MU_Event_Source_t *source);

bool MU_Event_Loop_run(MU_Event_Loop_t *loop);

bool MU_Event_Loop_stop(MU_Event_Loop_t *loop);

bool MU_Event_Loop_destroy(MU_Event_Loop_t *loop, bool free_sources);

#endif /* endif MU_EVENT_LOOP_H */