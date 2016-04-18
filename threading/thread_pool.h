#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stdbool.h>
#include <stddef.h>

#include "../io/logger.h"

struct c_utils_result;

/*
	thread_pool_t allows the user to submit a plethora of tasks to a queue of workers. Not only do these
	tasks get processed asynchronously in the background, they can also be prioritized to determine which
	tasks get processed before others. Tasks submitted can also optionally return the result from the given
	task asynchronously, for the caller to block on until the task has finished being processed.
*/

struct c_utils_thread_pool;

struct c_utils_thread_pool_conf {
	int flags;
	size_t num_threads;
	struct c_utils_logger *logger;
};

#define C_UTILS_THREAD_POOL_NO_TIMEOUT -1

#define C_UTILS_THREAD_POOL_RC_INSTANCE 1 << 0

/*
	Priorities work by directly associating it's priority to the amount of milliseconds it subtracts
	off of it's priority (Contrary to what intuition yields, the highest priority tasks will actually
	be seen to have the lowest priority), hence a task of HIGHEST priority WILL jump in front of a LOWEST
	priority task that's been submitted less than exactly 1000 milliseconds, or 1 entire second, ago. And,
	this prevents starvation as a task of LOWEST priority will wait no longer than 1 whole second to wait
	until it gets to be processed fairly.

	This can be circumvented for certain tasks by submitting them as IMMEDIATE, meaning that this wil jump
	ahead of another task regardless of it's priority, even if it is an IMMEDIATE task itself. This allows
	the caller to inject urgent tasks.
*/

#define C_UTILS_THREAD_POOL_PRIORITY_IMMEDIATE -1

#define C_UTILS_THREAD_POOL_PRIORITY_HIGHEST 1000

#define C_UTILS_THREAD_POOL_PRIORITY_HIGH 750

#define C_UTILS_THREAD_POOL_PRIORITY_MEDIUM 500

#define C_UTILS_THREAD_POOL_PRIORITY_LOW 250

#define C_UTILS_THREAD_POOL_PRIORITY_LOWEST 0

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_thread_pool thread_pool_t;
typedef struct c_utils_result result_t;

/*
	Enumerations & Constants
*/
#define NO_RESULT C_UTILS_NO_RESULT
#define LOWEST_PRIORITY C_UTILS_LOWEST_PRIORITY
#define LOW_PRIORITY C_UTILS_LOW_PRIORITY
#define HIGH_PRIORITY C_UTILS_HIGH_PRIORITY
#define HIGHEST_PRIORITY C_UTILS_HIGHEST_PRIORITY

/*
	Functions
*/
#define thread_pool_create(...) c_utils_thread_pool_create(__VA_ARGS__)
#define thread_pool_add(...) c_utils_thread_pool_add(__VA_ARGS__)
#define thread_pool_clear(...) c_utils_thread_pool_clear(__VA_ARGS__)
#define thread_pool_pause(...) c_utils_thread_pool_pause(__VA_ARGS__)
#define thread_pool_resume(...) c_utils_thread_pool_resume(__VA_ARGS__)
#define thread_pool_wait(...) c_utils_thread_pool_wait(__VA_ARGS__)
#define thread_pool_destroy(...) c_utils_thread_pool_destroy(__VA_ARGS__)
#define result_get(...) c_utils_result_get(__VA_ARGS__)
#define result_destroy(...) c_utils_result_destroy(__VA_ARGS__)
#endif



struct c_utils_thread_pool *c_utils_thread_pool_create();

/**
 * Returns a newly allocated thread pool with pool_size worker threads.
 * @param pool_size Number of worker threads, minimum of 1.
 * @return The thread pool instance, or NULL on allocation error or initialization error.
 */
struct c_utils_thread_pool *c_utils_thread_pool_create_conf(struct c_utils_thread_pool_conf *conf);

/**
 * Adds a task to the thread pool. While it requires a void *(*callback)(void *args)
 * function pointer, casting a function to void *, like (void *)function, will work
 * but has undefined behavior.
 * @param task Callback function to be called as the task.
 * @param args Arguments to pass to the thread pool.
 * @param flags LOWEST_PRIORITY | LOW_PRIORITY | HIGH_PRIORITY | HIGHEST_PRIORITY | NO_RESULT.
 * @return The result from the task to be obtained later or NULL if NO_RESULT.
 */
void c_utils_thread_pool_add(struct c_utils_thread_pool *tp, void *(*task)(void *), void *args, int priority);

struct c_utils_result *c_utils_thread_pool_add_for_result(struct c_utils_thread_pool *tp, void *(*task)(void *), void *args, int priority);

/**
 * Destroys the Result from a task.
 * @param result Result to be destroyed.
 * @return true on success, false if not.
 */
void c_utils_result_destroy(struct c_utils_result *result);

/**
* Clears the thread pool of all tasks.
* @param tp Thread Pool instance.
* @return true if successful, false if not.
*/
bool c_utils_thread_pool_clear(struct c_utils_thread_pool *tp);

/**
 * Destroys the current thread pool, waiting for the current tasks to be finished.
 * @return 1 on success.
 */
void c_utils_thread_pool_destroy(struct c_utils_thread_pool *tp);

/**
 * Obtain the result from the task, blocking until it is ready or until the time 
 * elapses, in which it will return NULL.
 * @param result Result to obtain result from.
 * @param timeout Maximum amount of time to block for if result isn't ready.
 * @return The item stored inside the result, or NULL if timeout or thread pool is shutdown.
 */
void *c_utils_result_get(struct c_utils_result *result, long long int timeout);

/**
 * Block until all tasks are finished or timeout ellapses.
 * @param tp Thread pool instance.
 * @param timeout The max amount of time to block for, infinite if -1.
 * @return true if the thread pool is finished, false if not.
 */
void c_utils_thread_pool_wait(struct c_utils_thread_pool *tp, long long int timeout);

/**
 * Pause the thread pool, and prevent worker threads from working. It does not stop threads from executing
 * their current tasks.
 * @param tp Thread pool instance.
 * @param timeout The max amount of time to block for, infinite if -1.
 * @return true if paused, false if not.
 */
void c_utils_thread_pool_pause_for(struct c_utils_thread_pool *tp, long long int timeout);

void c_utils_thread_pool_pause_until(struct c_utils_thread_pool *tp, struct timeval *timeout);

/**
 * Resume all worker threads that are paused.
 * @param tp Thread pool instance.
 * @return true on success, false if not.
 */
void c_utils_thread_pool_resume(struct c_utils_thread_pool *tp);

#endif /* END THREAD_POOL_H */