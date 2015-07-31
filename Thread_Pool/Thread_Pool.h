#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/*
 *  To summarize, a thread pool is a pool of threads that initialized at creation,
 * and perform tasks; by initializing a pool a threads beforehand, it greatly reduces
 * the overhead of creating and deleting a thread for each task. 
 * 
 * Now that that's clear, how does my thread pool differ? My thread pool features
 * selective pausing of tasks and pausing all tasks and resuming them at a later time
 * either by a timer or manually resuming the thread pool. My thread pool also features
 * the ability to obtain the result from a task and is optional.
 * It also gives the ability to wait for all tasks to finish, and alternatively to
 * wait up to a certain amount of time.
 * 
 * The thread pool currently is a static member, hence there can be only one at
 * any given time, however at a later date there will instead be a static table
 * of thread pools to allow multiple, however as that would complicate development,
 * I decided to push that off to a later date.
 * 
 * The thread pool also is static in another sense. The amount of threads will always
 * have the same amount of threads at runtime, and will only be reduced (to 0) on deletion.
 * 
 * The Thread Pool can be initialized with the below macros, used for bitmasking.
 * For example, to add a task where you want no result, you would do the following:
 * 
 * Thread_Pool_Add_Task(function, arguments, TP_NO_RESULT);
 * 
 * If you wanted to add a task where you did want the result, but you wanted
 * it of high priority and not being able to pause midway, you would do the following:
 * 
 * Thread_Pool_Add_Task(function, arguments, TP_NO_PAUSE | TP_HIGH_PRIORITY);
 * 
 * Alternatively, if you want to add a normal task (medium priority), you do the following:
 * 
 * Thread_Pool_Add_Task(function, arguments, TP_NONE);
 * 
 * Note that you do not need to specify everything, as in whether you want the result
 * or not, as there are set defaults, listed below. These defaults are there to make
 * adding general tasks easier.
 * 
 * Defaults:
 * 
 * - Result returned
 * - Will pause mid-task.
 * - Medium Priority task.
 */
/// The default behavior for a function.
#define TP_NONE 1 << 0
/// Will not return a Result upon submitting a task.
#define TP_NO_RESULT 1 << 1
/// The flagged task will complete before the thread working on it pauses.
#define TP_NO_PAUSE 1 << 2
/// Flags the task as lowest priority.
#define TP_LOWEST_PRIORITY 1 << 3
/// Flags the task as low priority.
#define TP_LOW_PRIORITY 1 << 4
/// Flags the task as high priority.
#define TP_HIGH_PRIORITY 1 << 5
/// Flags the task as highest priority.
#define TP_HIGHEST_PRIORITY 1 << 6

#include <PBQueue.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>
#include <MU_Events.h>
#include <MU_Arg_Check.h>
#include <MU_Logger.h>

typedef void *(*TP_Callback)(void *args);

typedef enum {
	/// Lowest possible priority
	TP_LOWEST,
	/// Low priority, above Lowest.
	TP_LOW,
	/// Medium priority, considered the "average".
	TP_MEDIUM,
	/// High priority, or "above average".
	TP_HIGH,
	/// Highest priority, or "imminent"
	TP_HIGHEST
} TP_Priority_e;

typedef struct {
	/// Determines if result is ready.
	MU_Event_t *is_ready;
	/// The returned item from the task.
	void *retval;
} TP_Result_t;

typedef struct TP_Task_t{
	/// Task to be executed.
	TP_Callback callback;
	/// Arguments to be passed to the task.
	void *args;
	/// Result from the Task.
	TP_Result_t *result;
	/// Priority of task.
	TP_Priority_e priority;
} TP_Task_t;

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
	PBQueue *queue;
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
 * Constructor for the thread pool, creating the requested amount of threads. 
 * There can only be one thread pool instantiated at any given time, so this function
 * will return 0 if there is another thread pool instantiated.
 * @param number_of_threads Amount of worker threads to initialize at startup.
 * @return 1 on success, 0 if the thread pool already exists.
 */
TP_Pool_t *TP_Pool_create(size_t pool_size);

/**
 * Adds a task to the thread pool. While it requires a void *(*callback)(void *args)
 * function pointer, casting a function to void *, like (void *)function, will work
 * but has undefined behavior.
 * @param callback Callback function to be called as the task.
 * @param args Arguments to pass to the thread pool.
 * @param flags TP_NONE | TP_LOWEST_PRIORITY | TP_LOW_PRIORITY | TP_HIGH_PRIORITY
 * | TP_HIGHEST_PRIORITY | TP_NO_RESULT | TP_NO_PAUSE.
 * @return The result from the task to be obtained later or NULL if TP_NO_RESULT.
 */
TP_Result_t *TP_Pool_add(TP_Pool_t *tp, TP_Callback callback, void *args, int flags);

/**
 * Clear all tasks in the task queue. Note: Will not cancel currently run tasks.
 * @return 1 on success.
 */
bool TP_Pool_clear(TP_Pool_t *tp);

/**
 * Destroys the Result from a task.
 * @param result Result to be destroyed.
 * @return 1 on success.
 */
bool TP_Result_destroy(TP_Result_t *result);

/**
 * Destroys the current thread pool, waiting for the current tasks to be finished.
 * @return 1 on success.
 */
bool TP_Pool_destroy(TP_Pool_t *tp);

/**
 * Obtain the result from the task, blocking until it is ready or until the time 
 * elapses, in which it will return NULL.
 * @param result Result to obtain result from.
 * @param seconds Maximum amount of time to block for if result isn't ready.
 * @return The item stored inside the result, or NULL if timeout or thread pool is shutdown.
 */
void *TP_Result_get(TP_Result_t *result, long long int timeout);

/**
 * Block until all tasks are finished or timeout ellapses.
 * @return 1 on success.
 */
bool TP_Pool_wait(TP_Pool_t *tp, long long int timeout);

/**
 * Pause all operations in the task queue. Even NO_PAUSE flagged tasks will enter
 * the pause signal handler, to determine if it can return, but it will return immediately
 * once it determines whether it is or not. Primitive blocking functions like
 * sleep and select should return perfectly since SA_RESTART is flagged in sigaction.
 * @return 1 on success, 0 if at least one thread is not able to pause.
 * @param seconds Amount of time to pause for.
 * @return 1 on success, 0 if at least one thread cannot pause.
 */
bool TP_Pool_pause(TP_Pool_t *tp, long long int timeout);

/**
 * Resume all worker threads that are paused. If the thread pool is not paused, or it is
 * shutdown, it returns 0.
 * @return 1 on success, 0 if at least one thread cannot be resumed, thread pool is not paused
 * or does not exist..
 */
bool TP_Pool_resume(TP_Pool_t *tp);

#endif /* END THREAD_POOL_H */