#ifndef THREAD_POOL_H
#define THREAD_POOL_H


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
	LOWEST_PRIORITY,
	/// Low priority, above Lowest.
	LOW_PRIORITY,
	/// Medium priority, considered the "average".
	MEDIUM_PRIORITY,
	/// High priority, or "above average".
	HIGH_PRIORITY,
	/// Highest priority, or "imminent"
	HIGHEST_PRIORITY
};

enum Pause_Preference{
	/// Task can never be paused, must always finish.
	NO_PAUSE,
	/// Task can be paused before it finishes.
	PAUSE
};

/// Determines whether a task is paused
enum Task_Status {
	/// The thread running the task will pause after it finishes.
	DELAYED_PAUSE,
	/// Task has been aborted, either due to user request or segmentation fault.
	ABORTED,
	/// Task is being processed.
	PROCESSING,
	/// Task is waiting in queue.
	WAITING
};

struct Thread_Pool {
	/// Array of threads.
	pthread_t **threads;
	/// The queue with all jobs assigned to it.
	Task_Queue *queue;
	/// Amount of threads currently created, A.K.A Max amount.
	size_t thread_count;
	/// Amount of threads currently active.
	size_t active_threads;
	/// Flag to keep all threads alive.
	volatile unsigned char keep_alive;
	/// Flag used to pause all threads
	volatile unsigned char paused;
	/// Mutex for thread_count and active_threads, and keep_alive.
	pthread_mutex_t *thread_count_change;
	/// Condition variable used to resume all threads.
	pthread_cond_t *resume;
	/// Mutex accompanied by the condition variable.
	pthread_mutex_t *pause;
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
	thread_callback cb;
	/// Arguments to be passed to the task.
	void *args;
	/// Pointing to the next task in the queue.
	Task *next;
	/// Mutex to ensure no other thread attempts to do this task.
	pthread_mutex_t *being_processed;
	/// Result from the Task.
	Result *result;
	/// Priority of task.
	Priority priority;
	/// Determines whether or not this task can be paused midway.
	Pause_Preference preference;
	/// Determines the status of the task. Mostly used for determining the action to do before and after task is completed.
	Task_Status status;
	/// Time added to the thread pool, used to determine whether or not a thread should be terminated if the difference between then and now is 60 seconds.
	Time_t *time_added;
};

/**
 * The queue all worker threads use to obtain the next task to process.
 */
struct Task_Queue{
	/// Pointer to the head of the queue.
	Task *head;
	/// Pointer to the tail of the queue.
	Task *tail;
	/// Maintains the size of the current queue.
	size_t size;
	/// Condition variable to signal that all tasks are finished.
	pthread_cond_t *is_finished;
	/// Lock to use to wait until is_finished is signaled.
	pthread_mutex_t *no_tasks;
	/// Condition variable to signal when a task is added to the queue.
	pthread_cond_t *new_task;
	/// Lock to use to wait until a new task is added.
	pthread_mutex_t *await_task;
	/// Lock to use when adding a task.
	pthread_mutex_t *adding_task;
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
 * @param cb Callback function to be called as the task.
 * @param args Arguments to pass to the thread pool.
 * @return The result from the task to be obtained later.
 */
Result *Thread_Pool_Add_Task(thread_callback cb, void *args, Priority priority, Pause_Preference preference);

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

/**
 * Blocks until all tasks in the thread pool are finished.
 */
void Thread_Pool_Wait(void);

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