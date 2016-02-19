#ifndef MU_EVENT_LOOP_H
#define MU_EVENT_LOOP_H

#include <stdbool.h>
#include <sys/time.h>
#include <DS_List.h>
#include <MU_Events.h>

typedef void *(*MU_Event_Prepare)();
typedef bool (*MU_Event_Check)(void *);
typedef bool (*MU_Event_Dispatch)(void *);
typedef bool (*MU_Event_Finalize)(void *);

typedef struct {
	/// Maintains list of sources to check.
	DS_List_t *sources;
	/// Keep-Alive flag
	_Atomic bool keep_alive;
	/// The event to wait on for it to finish.
	MU_Event_t *finished;
} MU_Event_Loop_t;

/*
	void*::MU_Event_Prepare(Void); Prepares the event.
	bool::MU_Event_Check(Void*); Checks if event is ready. 
	bool::MU_Event_Dispatch(Void*); Dispatches the event.
	bool::MU_Event_Finalize(Void*); Destroys the data if it is finished.
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

/**
 * 
 * @param prepare_cb
 * @param check_cb
 * @param dispatch_cb
 * @param finalize_cb
 * @param timeout
 * @return 
 */
MU_Event_Source_t *MU_Event_Source_create(MU_Event_Prepare prepare_cb, MU_Event_Check check_cb, MU_Event_Dispatch dispatch_cb, MU_Event_Finalize finalize_cb, unsigned long long int timeout);

/**
 * 
 * @param source
 * @return 
 */
bool MU_Event_Source_destroy(MU_Event_Source_t *source);

/**
 * 
 * @return 
 */
MU_Event_Loop_t *MU_Event_Loop_create(void);

/**
 * 
 * @param loop
 * @param source
 * @return 
 */
bool MU_Event_Loop_add(MU_Event_Loop_t *loop, MU_Event_Source_t *source);

/**
 * 
 * @param loop
 * @return 
 */
bool MU_Event_Loop_run(MU_Event_Loop_t *loop);

/**
 * 
 * @param loop
 * @return 
 */
bool MU_Event_Loop_stop(MU_Event_Loop_t *loop);

/**
 * 
 * @param loop
 * @param free_sources
 * @return 
 */
bool MU_Event_Loop_destroy(MU_Event_Loop_t *loop, bool free_sources);

#endif /* endif MU_EVENT_LOOP_H */