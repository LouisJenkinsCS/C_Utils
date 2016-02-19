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
<<<<<<< HEAD
 * 
 * @param prepare_cb
 * @param check_cb
 * @param dispatch_cb
 * @param finalize_cb
 * @param timeout
 * @return 
=======
 * Creates an event source, which is an event to be polled on and executed when their conditions are
 * right. If prepare_cb is null, it will not prepare any data. If check_cb, it would act as if it
 * had returned true. If dispatch_cb is null, no event source will be created. If finalize_cb is null,
 * then the data itself will never be deleted/finalized. Finally, if timeout is 0, then it will not
 * function as a timed event, otherwise it will.
 * @param prepare_cb Prepares the data.
 * @param check_cb Checks the data.
 * @param dispatch_cb Dispatches the data.
 * @param finalize_cb Finalizes the data.
 * @param timeout If not 0, then it will only be checked and dispatched based on this.
 * @return Configured event source.
>>>>>>> development
 */
MU_Event_Source_t *MU_Event_Source_create(MU_Event_Prepare prepare_cb, MU_Event_Check check_cb, MU_Event_Dispatch dispatch_cb, MU_Event_Finalize finalize_cb, unsigned long long int timeout);

/**
<<<<<<< HEAD
 * 
 * @param source
 * @return 
=======
 * Destroys the event source.
 * @param source Source.
 * @return True if successful, false if source is null.
>>>>>>> development
 */
bool MU_Event_Source_destroy(MU_Event_Source_t *source);

/**
<<<<<<< HEAD
 * 
 * @return 
=======
 * Creates an event loop, with no sources. It will initialize it's data.
 * @return Event loop.
>>>>>>> development
 */
MU_Event_Loop_t *MU_Event_Loop_create(void);

/**
<<<<<<< HEAD
 * 
 * @param loop
 * @param source
 * @return 
=======
 * Addsa source to the loop.
 * @param loop Loop.
 * @param source Source.
 * @return True if successful, false is loop or source is null.
>>>>>>> development
 */
bool MU_Event_Loop_add(MU_Event_Loop_t *loop, MU_Event_Source_t *source);

/**
<<<<<<< HEAD
 * 
 * @param loop
 * @return 
=======
 * Runs the event loop, polling on it every 10ms. Warning, this should be run on a separate
 * thread, as this will block until completion.
 * @param loop Loops.
 * @return True if successful, false is loop is already running or loop is null.
>>>>>>> development
 */
bool MU_Event_Loop_run(MU_Event_Loop_t *loop);

/**
<<<<<<< HEAD
 * 
 * @param loop
 * @return 
=======
 * Stops the loop. The current iteration is completed before exiting.
 * @param loop Loops.
 * @return True if successful, false is loop is not running or is null.
>>>>>>> development
 */
bool MU_Event_Loop_stop(MU_Event_Loop_t *loop);

/**
<<<<<<< HEAD
 * 
 * @param loop
 * @param free_sources
 * @return 
=======
 * Stops and then Destroys the event loop, as well as freeing all sources if free_sources is true.
 * @param loop Loops.
 * @param free_sources To free all sources or not.
 * @return True if successful, false if loop is null or unable to free all sources.
>>>>>>> development
 */
bool MU_Event_Loop_destroy(MU_Event_Loop_t *loop, bool free_sources);

#endif /* endif MU_EVENT_LOOP_H */