#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/**
* Invokes the default behavior for the function. Defaults are listed below...
*
* Thread_Pool_Add_Task:
* - Medium Priority.
* - Pausable.
* - Get result.
*/
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

#include "PBQueue.h"

/*
* Example of parameter usage: Lets say you wish to add a task of rather high importance, and it's crucial that
* it does not pause in the middle of it, but you also do not want the result. You can easily do such a thing, without
* having to deal with having to deal with a large amount of parameters. 
*
* Thread_Pool_Add_Task(Task, Args, NO_PAUSE | NO_RESULT | HIGHEST_PRIORITY);
*
* Instead of something like Thread_Pool_Add_Task(Task, Args, 1, 1, 4);
*
* Note above that this assumes that Add_Task would have a int (or enumeration) for each option, imagine you want to add
* a default task. A task of no particular importance, one where you wish to keep the result, and you don't care if the task is paused
* or not.
*
* Thread_Pool_Add_Task(Task, Args, TP_NONE);
*
* Instead of something like Thread_Pool_Add_Task(Task, Args, 0, 0, 2);
*
* It gets even more messy with enumerations.
*
* Thread_Pool_Add_Task(Task, Args, PAUSABLE, GET_RESULT, MEDIUM_PRIORITY);
*
* Or more practically, a combination of the two.
*
* Thread_Pool_Add_Task(Task, Args, 1, 1, MEDIUM_PRIORITY);
*
* Regardless, the bitwise one looks more pretty and elegant.
*/

typedef struct Worker Worker;

typedef struct Thread_Pool Thread_Pool;

typedef struct Result Result;

typedef struct Sub_Process Sub_Process;

typedef struct Task Task;

typedef struct Task_Queue Task_Queue;

typedef enum Priority Priority;

typedef enum Pause_Preference Pause_Preference;

typedef enum Task_Status Task_Status;

typedef void *(*thread_callback)(void *args);

/// For insertions in a priority blocking queue fashion.
enum Priority{
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
};

struct Worker {
	/// The worker thread that does the work.
	pthread_t *thread;
	/// The worker thread id.
	unsigned int thread_id;
	/// The current task being worked on.
	Task *task;
};

struct Thread_Pool {
	/// Array of threads.
	Worker **worker_threads;
	/// The queue with all jobs assigned to it.
	PBQueue *queue;
	/// Amount of threads currently created, A.K.A Max amount.
	size_t thread_count;
	/// Amount of threads currently active.
	size_t active_threads;
	/// Flag to keep all threads alive.
	volatile unsigned char keep_alive;
	/// Flag used to pause all threads
	volatile unsigned char paused;
	/// Used for timed pauses.
	volatile unsigned int seconds_to_pause;
	/// Mutex for thread_count and active_threads, and keep_alive.
	pthread_mutex_t *thread_count_change;
	/// Condition variable used to resume all threads.
	pthread_cond_t *resume;
	/// Mutex accompanied by the condition variable.
	pthread_mutex_t *is_paused;
	/// Condition variable to determine whether all tasks are finished.
	pthread_cond_t *all_tasks_finished;
	/// Mutex accompanied by all_tasks_finished
	pthread_mutex_t *no_tasks;
};

/**
 * The result returned when adding a new task to the task queue. It allows you to
 * to wait on it's result and obtain it.
 */
struct Result {
	/// Lock to protect contents of 'Result'
	pthread_mutex_t *not_ready;
	/// Condition variable to signal result being ready.
	pthread_cond_t *is_ready;
	/// Determines whether or not it has been processed.
	volatile unsigned char ready;
	/// The return type, NULL until ready.
	void *item;
};

/**
 * Encompasses the task submitted by the user. Contains the result of the task as well.
 */
struct Task {
	/// Task to be executed.
	thread_callback callback;
	/// Arguments to be passed to the task.
	void *args;
	/// Pointing to the next task in the queue.
	Task *next;
	/// Result from the Task.
	Result *result;
	/// Priority of task.
	Priority priority;
	/// Determines whether or not this task can be paused midway.
	unsigned char no_pause;
	/// Determines the status of the task. Mostly used for determining the action to do before and after task is completed.
	unsigned char delayed_pause;
};

/**
 * Constructor for the thread pool, creating the requested amount of threads. 
 * Used to initialize the static variable in the source file, and should not
 * be called (yet, no checks for if the thread pool currently exists are 
 * implemented yet) if one already exists, as it may result in undefined behavior.
 * @param number_of_threads Amount of worker threads to initialize at startup.
 * @return 1 on success.
 */
int Thread_Pool_Init(size_t number_of_threads);

/**
 * Adds a task to the thread pool. Callback must return a pointer * and arguments must 
 * also be a pointer. Alternatively, you can cast the function pointer to a void pointer
 * like such: (void *)function; Doing so results in undefined behavior, but allows
 * it to work if you do not really want the result.
 * @param callback Callback function to be called as the task.
 * @param args Arguments to pass to the thread pool.
 * @return The result from the task to be obtained later.
 */
Result *Thread_Pool_Add_Task(thread_callback callback, void *args, int flags);

/**
 * Destroys the Result from a task.
 * @param result Result to be destroyed.
 * @return 1 on success.
 */
int Thread_Pool_Result_Destroy(Result *result);

/**
 * Destroys the current thread pool, waiting for the current tasks to be finished.
 * @return 1 on success.
 */
int Thread_Pool_Destroy(void);

/**
 * Obtain the result from a Result, blocking until it's ready. This is the only safe
 * way to obtain said result.
 * @param result Result to obtain result from.
 * @return The item stored inside of the result.
 */ 
void *Thread_Pool_Obtain_Result(Result *result);

void *Thread_Pool_Timed_Obtain_Result(Result *result, unsigned int seconds);

/**
 * Blocks until all tasks in the thread pool are finished.
 */
void Thread_Pool_Wait(void);

void Thread_Pool_Timed_Wait(unsigned int seconds);

/**
 * Pause all operations in the task queue. Warning: May be unstable if critical operations
 * are being performed.
 * @return 1 on success, 0 if at least one thread is not able to pause.
 */
int Thread_Pool_Pause(void);

/**
 * (Unimplemented) Pause the thread pool for the given amount of time.
 * @param seconds Amount of time to pause for.
 * @return 1 on success, 0 if at least one thread cannot pause.
 */
int Thread_Pool_Timed_Pause(unsigned int seconds);

/**
 * Resume all worker threads that are paused. If no threads are waiting, nothing
 * happens.
 * @return 1 on success, 0 if at least one thread cannot be resumed.
 */
int Thread_Pool_Resume(void);
#endif /* END THREAD_POOL_H */