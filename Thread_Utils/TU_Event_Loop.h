#ifndef TU_EVENT_LOOP_H
#define TU_EVENT_LOOP_H

#include <stdbool.h>
#include <sys/time.h>
#include <DS_List.h>
#include <TU_Events.h>

#ifdef C_UTILS_USE_POSIX_STD
#define event_loop_t TU_Event_Loop_t
#define event_source_t TU_Event_Source_t
#define event_loop_create(...) TU_Event_Loop_create(__VA_ARGS__)
#define event_source_create(...) TU_Event_Source_create(__VA_ARGS__)
#define event_loop_add(...) TU_Event_Loop_add(__VA_ARGS__)
#define event_loop_run(...) TU_Event_Loop_run(__VA_ARGS__)
#define event_loop_stop(...) TU_Event_Loop_stop(__VA_ARGS__)
#define event_source_destroy(...) TU_Event_Source_destroy(__VA_ARGS__)
#define event_loop_destroy(...) TU_Event_Loop_destroy(__VA_ARGS__)
#endif

typedef void *(*TU_Event_Prepare)();
typedef bool (*TU_Event_Check)(void *);
typedef bool (*TU_Event_Dispatch)(void *);
typedef bool (*TU_Event_Finalize)(void *);

typedef struct {
	/// Maintains list of sources to check.
	DS_List_t *sources;
	/// Keep-Alive flag
	_Atomic bool keep_alive;
	/// The event to wait on for it to finish.
	TU_Event_t *finished;
} TU_Event_Loop_t;

/*
	void*::TU_Event_Prepare(Void); Prepares the event.
	bool::TU_Event_Check(Void*); Checks if event is ready. 
	bool::TU_Event_Dispatch(Void*); Dispatches the event.
	bool::TU_Event_Finalize(Void*); Destroys the data if it is finished.
*/
typedef struct {
	/// Data returned from an event.
	void *data;
	/// Callback used to prepare the data passed to it.
	TU_Event_Prepare prepare;
	/// Callback used to check if event is ready.
	TU_Event_Check check;
	/// Callback used to dispatch said event.
	TU_Event_Dispatch dispatch;
	/// Callback used whenever the main loop finishes.
	TU_Event_Finalize finalize;
	/// Internal flags used to help maintain information about the event source.
	unsigned int flags;
	/// The timeval used to record the absolute time of the next timeout.
	struct timeval next_timeout;
	/// The timeout the timeval will be set to after triggering.
	unsigned int timeout;
} TU_Event_Source_t;

/**
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
 */
TU_Event_Source_t *TU_Event_Source_create(TU_Event_Prepare prepare_cb, TU_Event_Check check_cb, TU_Event_Dispatch dispatch_cb, TU_Event_Finalize finalize_cb, unsigned long long int timeout);

/**
 * Destroys the event source.
 * @param source Source.
 * @return True if successful, false if source is null.
 */
bool TU_Event_Source_destroy(TU_Event_Source_t *source);

/**
 * Creates an event loop, with no sources. It will initialize it's data.
 * @return Event loop.
 */
TU_Event_Loop_t *TU_Event_Loop_create(void);

/**
 * Addsa source to the loop.
 * @param loop Loop.
 * @param source Source.
 * @return True if successful, false is loop or source is null.
 */
bool TU_Event_Loop_add(TU_Event_Loop_t *loop, TU_Event_Source_t *source);

/**
 * Runs the event loop, polling on it every 10ms. Warning, this should be run on a separate
 * thread, as this will block until completion.
 * @param loop Loops.
 * @return True if successful, false is loop is already running or loop is null.
 */
bool TU_Event_Loop_run(TU_Event_Loop_t *loop);

/**
 * Stops the loop. The current iteration is completed before exiting.
 * @param loop Loops.
 * @return True if successful, false is loop is not running or is null.
 */
bool TU_Event_Loop_stop(TU_Event_Loop_t *loop);

/**
 * Stops and then Destroys the event loop, as well as freeing all sources if free_sources is true.
 * @param loop Loops.
 * @param free_sources To free all sources or not.
 * @return True if successful, false if loop is null or unable to free all sources.
 */
bool TU_Event_Loop_destroy(TU_Event_Loop_t *loop, bool free_sources);

#endif /* endif TU_EVENT_LOOP_H */