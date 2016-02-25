#ifndef TU_EVENT_LOOP_H
#define TU_EVENT_LOOP_H

#include <stdbool.h>

struct c_utils_event_loop;
struct c_utils_event_source;

#ifdef NO_C_UTILS_PREFIX
typedef struct c_utils_event_loop event_loop_t;
typedef struct c_utils_event_source event_source_t;
#define event_loop_create(...) c_utils_event_loop_create(__VA_ARGS__)
#define event_source_create(...) c_utils_event_source_create(__VA_ARGS__)
#define event_loop_add(...) c_utils_event_loop_add(__VA_ARGS__)
#define event_loop_run(...) c_utils_event_loop_run(__VA_ARGS__)
#define event_loop_stop(...) c_utils_event_loop_stop(__VA_ARGS__)
#define event_source_destroy(...) c_utils_event_source_destroy(__VA_ARGS__)
#define event_loop_destroy(...) c_utils_event_loop_destroy(__VA_ARGS__)
#endif

typedef void *(*c_utils_event_prepare)();
typedef bool (*c_utils_event_check)(void *);
typedef bool (*c_utils_event_dispatch)(void *);
typedef bool (*c_utils_event_finalize)(void *);

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
struct c_utils_event_source *c_utils_event_source_create(c_utils_event_prepare prepare_cb, c_utils_event_check check_cb, c_utils_event_dispatch dispatch_cb, c_utils_event_finalize finalize_cb, unsigned long long int timeout);

/**
 * Destroys the event source.
 * @param source Source.
 * @return True if successful, false if source is null.
 */
bool c_utils_event_source_destroy(struct c_utils_event_source *source);

/**
 * Creates an event loop, with no sources. It will initialize it's data.
 * @return Event loop.
 */
struct c_utils_event_loop *TU_Event_Loop_create(void);

/**
 * Addsa source to the loop.
 * @param loop Loop.
 * @param source Source.
 * @return True if successful, false is loop or source is null.
 */
bool c_utils_event_loop_add(struct c_utils_event_loop *loop, struct c_utils_event_source *source);

/**
 * Runs the event loop, polling on it every 10ms. Warning, this should be run on a separate
 * thread, as this will block until completion.
 * @param loop Loops.
 * @return True if successful, false is loop is already running or loop is null.
 */
bool c_utils_event_loop_run(struct c_utils_event_loop *loop);

/**
 * Stops the loop. The current iteration is completed before exiting.
 * @param loop Loops.
 * @return True if successful, false is loop is not running or is null.
 */
bool c_utils_event_loop_stop(struct c_utils_event_loop *loop);

/**
 * Stops and then Destroys the event loop, as well as freeing all sources if free_sources is true.
 * @param loop Loops.
 * @param free_sources To free all sources or not.
 * @return True if successful, false if loop is null or unable to free all sources.
 */
bool c_utils_event_loop_destroy(struct c_utils_event_loop *loop, bool free_sources);

#endif /* endif TU_EVENT_LOOP_H */