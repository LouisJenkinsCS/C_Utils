#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/*
 *  A simple, yet powerful Thread Pool implementation.
 * 
 * This thread pool differs from other's in the fact that not only does it use a 
 * priority queue for tasks, it also allows you to the obtain the result from an operation
 * if desired and wait on them through my own implementations of Win32 Events.
 * 
 * It also uses bit flag passing with the bit-wise OR operator (|), to fine-tune
 * the usage of the thread pool.
 * 
 * For example, if you wanted to add a task of above average (high) priority, but you
 * do want the result, you simply pass the flag TP_HIGH_PRIORITY. However, lets say you 
 * do not want the result but still want it to be high priority, then it's as simple
 * as TP_HIGH_PRIORITY | TP_NO_RESULT. 
 * 
 * Now, lets say you don't want to deal with prioritized tasks and want the result, which
 * is the default, you can simply pass TP_NONE or 0 for flags.
 * 
 * Finally it has the ability to pause and resume the thread pool, or even pause
 * the thread pool for a specified amount of time. Trivial, yet useful in certain situations.
 * You can even wait on the thread pool, with the option of a timeout, allowing you to
 * finetune whether you want to block indefinitely or poll.
 * 
 * The default behavior for the thread pool is the following...
 * - A TP_Result_t object is returned
 * - The task is added with a normal/medium priority
 * - You MUST free the TP_Result_t object when finished.
 */

/// The default behavior for a function.
#define TP_NONE 1 << 0
/// Will not return a Result upon submitting a task.
#define TP_NO_RESULT 1 << 1
/// Flags the task as lowest priority.
#define TP_LOWEST_PRIORITY 1 << 2
/// Flags the task as low priority.
#define TP_LOW_PRIORITY 1 << 3
/// Flags the task as high priority.
#define TP_HIGH_PRIORITY 1 << 4
/// Flags the task as highest priority.
#define TP_HIGHEST_PRIORITY 1 << 5

typedef void *(*TP_Callback)(void *args);

#include <DS_PBQueue.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>
#include <MU_Events.h>

typedef struct {
	/// Determines if result is ready.
	MU_Event_t *is_ready;
	/// The returned item from the task.
	void *retval;
} TP_Result_t;

typedef struct {
	/// The worker thread that does the work.
	pthread_t *thread;
	/// The worker thread id.
	unsigned int thread_id;
} TP_Worker_t;

typedef struct {
	/// Array of threads.
	TP_Worker_t **worker_threads;
	/// The queue with all jobs assigned to it.
	DS_PBQueue_t *queue;
	/// Amount of threads currently created, A.K.A Max amount.
	_Atomic size_t thread_count;
	/// Amount of threads currently active.
	_Atomic size_t active_threads;
	/// Flag to keep all threads alive.
	volatile bool keep_alive;
	/// Used for timed pauses.
	volatile unsigned int seconds_to_pause;
	/// Used to signal an error occured during initialization
	volatile bool init_error;
	/// Event for pause/resume.
	MU_Event_t *resume;
	/// Event for if the thread pool is finished at the moment.
	MU_Event_t *finished;
} TP_Pool_t;

/**
 * Returns a newly allocated thread pool with pool_size worker threads.
 * @param pool_size Number of worker threads, minimum of 1.
 * @return The thread pool instance, or NULL on allocation error or initialization error.
 */
TP_Pool_t *TP_Pool_create(size_t pool_size);

/**
 * Adds a task to the thread pool. While it requires a void *(*callback)(void *args)
 * function pointer, casting a function to void *, like (void *)function, will work
 * but has undefined behavior.
 * @param callback Callback function to be called as the task.
 * @param args Arguments to pass to the thread pool.
 * @param flags TP_NONE | TP_LOWEST_PRIORITY | TP_LOW_PRIORITY | TP_HIGH_PRIORITY
 * | TP_HIGHEST_PRIORITY | TP_NO_RESULT.
 * @return The result from the task to be obtained later or NULL if TP_NO_RESULT.
 */
TP_Result_t *TP_Pool_add(TP_Pool_t *tp, TP_Callback callback, void *args, int flags);

/**
 * Destroys the Result from a task.
 * @param result Result to be destroyed.
 * @return true on success, false if not.
 */
bool TP_Result_destroy(TP_Result_t *result);

/**
* Clears the thread pool of all tasks.
* @param tp Thread Pool instance.
* @return true if successful, false if not.
*/
bool TP_Pool_clear(TP_Pool_t *tp);

/**
 * Destroys the current thread pool, waiting for the current tasks to be finished.
 * @return 1 on success.
 */
bool TP_Pool_destroy(TP_Pool_t *tp);

/**
 * Obtain the result from the task, blocking until it is ready or until the time 
 * elapses, in which it will return NULL.
 * @param result Result to obtain result from.
 * @param timeout Maximum amount of time to block for if result isn't ready.
 * @return The item stored inside the result, or NULL if timeout or thread pool is shutdown.
 */
void *TP_Result_get(TP_Result_t *result, long long int timeout);

/**
 * Block until all tasks are finished or timeout ellapses.
 * @param tp Thread pool instance.
 * @param timeout The max amount of time to block for, infinite if -1.
 * @return true if the thread pool is finished, false if not.
 */
bool TP_Pool_wait(TP_Pool_t *tp, long long int timeout);

/**
 * Pause the thread pool, and prevent worker threads from working. It does not stop threads from executing
 * their current tasks.
 * @param tp Thread pool instance.
 * @param timeout The max amount of time to block for, infinite if -1.
 * @return true if paused, false if not.
 */
bool TP_Pool_pause(TP_Pool_t *tp, long long int timeout);

/**
 * Resume all worker threads that are paused.
 * @param tp Thread pool instance.
 * @return true on success, false if not.
 */
bool TP_Pool_resume(TP_Pool_t *tp);

#endif /* END THREAD_POOL_H */