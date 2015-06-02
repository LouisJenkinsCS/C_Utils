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

#include "PBQueue.h"

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
	/// Determines whether the thread is properly setup.
	volatile unsigned char is_setup;
	/// The current task being worked on.
	Task *task;
};

struct Thread_Pool {
	/// Array of threads.
	Worker **worker_threads;
	/// The queue with all jobs assigned to it.
	PBQueue *queue;
	/// Amount of threads currently created, A.K.A Max amount.
	unsigned int thread_count;
	/// Amount of threads currently active.
	unsigned int active_threads;
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

struct Result {
	/// Lock to protect contents of 'Result'
	pthread_mutex_t *not_ready;
	/// Condition variable to signal result being ready.
	pthread_cond_t *is_ready;
	/// Determines whether or not it has been processed.
	volatile unsigned char ready;
	/// The returned item from the task.
	void *item;
};

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
	/// Determines whether or not the thread that runs this task should pause after.
	unsigned char delayed_pause;
};

/**
 * Constructor for the thread pool, creating the requested amount of threads. 
 * There can only be one thread pool instantiated at any given time, so this function
 * will return 0 if there is another thread pool instantiated.
 * @param number_of_threads Amount of worker threads to initialize at startup.
 * @return 1 on success, 0 if the thread pool already exists.
 */
int Thread_Pool_Init(size_t number_of_threads);

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
Result *Thread_Pool_Add_Task(thread_callback callback, void *args, int flags);

/**
 * Clear all tasks in the task queue. Note: Will not cancel currently run tasks.
 * @return 1 on success.
 */
int Thread_Pool_Clear_Tasks(void);

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
 * way to obtain said result. Also note, if the thread pool is shutdown, then the
 * result returns NULL.
 * @param result Result to obtain result from.
 * @return The item stored inside of the result or NULL if thread pool is shut down.
 */ 
void *Thread_Pool_Obtain_Result(Result *result);

/**
 * Obtain the result from the task, blocking until it is ready or until the time 
 * elapses, in which it will return NULL.
 * @param result Result to obtain result from.
 * @param seconds Maximum amount of time to block for if result isn't ready.
 * @return The item stored inside the result, or NULL if timeout or thread pool is shutdown.
 */
void *Thread_Pool_Timed_Obtain_Result(Result *result, unsigned int seconds);

/**
 * Block until all tasks are finished.
 * @return 1 on success.
 */
int Thread_Pool_Wait(void);

/**
 * Block until all tasks are finished or time elapses.
 * @param seconds Maximum amount of time to block for.
 * @return 1 if all tasks are finished, 0 if timeout.
 */
int Thread_Pool_Timed_Wait(unsigned int seconds);

/**
 * Pause all operations in the task queue. Even NO_PAUSE flagged tasks will enter
 * the pause signal handler, to determine if it can return, but it will return immediately
 * once it determines whether it is or not. Primitive blocking functions like
 * sleep and select should return perfectly since SA_RESTART is flagged in sigaction.
 * @return 1 on success, 0 if at least one thread is not able to pause.
 */
int Thread_Pool_Pause(void);

/**
 * Pause the thread pool for the given amount of time or resumed. Note: See: Thread_Pool_Pause.
 * @param seconds Amount of time to pause for.
 * @return 1 on success, 0 if at least one thread cannot pause.
 */
int Thread_Pool_Timed_Pause(unsigned int seconds);

/**
 * Resume all worker threads that are paused. If the thread pool is not paused, or it is
 * shutdown, it returns 0.
 * @return 1 on success, 0 if at least one thread cannot be resumed, thread pool is not paused
 * or does not exist..
 */
int Thread_Pool_Resume(void);
#endif /* END THREAD_POOL_H */