#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/// Normal, default option for a given function.
#define NONE 1 << 0
/// For asynchronous actions, will return immediately from a function instead of blocking.
#define ASYNC 1 << 1
/// Check if a parameter was passed
#define SELECTED(ARGUMENT, MACRO) ((ARGUMENT & MACRO))
/// Used to lock the given mutex.
#define LOCK(mutex) pthread_mutex_lock(mutex)
/// Used to try to lock the mutex, returning immediately on failure.
#define TRYLOCK(mutex) pthread_mutex_trylock(mutex)
/// Used to unlock the given mutex.
#define UNLOCK(mutex) pthread_mutex_unlock(mutex)
/// Causes the current thread to wait for a signal to be sent.
#define WAIT(condition, mutex) pthread_cond_wait(condition, mutex)
/// Signals a thread waiting on the condition based on a default scheduler.
#define SIGNAL(condition) pthread_cond_signal(condition)
/// Used to broadcast to all threads waiting on the condition variable.
#define BROADCAST(condition) pthread_cond_broadcast(condition)
/// Macro used to pause the thread, as the function name is very misleading. Sends SIGUSR1 signal.
#define PAUSE(thread) pthread_kill(thread, SIGUSR1)
/// Used to atomically increment the variable.
#define INCREMENT(var, mutex) \
		do { \
			LOCK(mutex); \
			var++; \
			UNLOCK(mutex); \
		} while(0)
/// Used to atomically decrement the variable.
#define DECREMENT(var, mutex) \
		do { \
			LOCK(mutex); \
			var--; \
			UNLOCK(mutex); \
		} while(0)
/// Used to quickly initialize a mutex.
#define INIT_MUTEX(mutex, attr) \
		    do { \
			   mutex = malloc(sizeof(pthread_mutex_t)); \
			   pthread_mutex_init(mutex, attr); \
			} while(0)
/// Used to quickly initialize a condition variable.
#define INIT_COND(cond, attr) \
			do { \
			   cond = malloc(sizeof(pthread_cond_t)); \
			   pthread_cond_init(cond, attr); \
			} while(0)
/// Free the mutex.
#define DESTROY_MUTEX(mutex) \
			do { \
				pthread_mutex_destroy(mutex); \
				free(mutex); \
			} while(0)
/// Free the condition variable.
#define DESTROY_COND(cond) \
			do { \
				pthread_cond_destroy(cond); \
				free(cond); \
			} while(0)
/// Simple macro to enable debug
#define TP_DEBUG 0
/// Print a message if and only if TP_DEBUG is enabled.
#define TP_DEBUG_PRINT(str) (TP_DEBUG ? printf(str) : TP_DEBUG)
/// Print a formatted message if and only if TP_DEBUG is enabled.
#define TP_DEBUG_PRINTF(str, ...)(TP_DEBUG ? printf(str, __VA_ARGS__) : TP_DEBUG)
typedef struct Thread_Pool Thread_Pool;

typedef struct Result Result;

typedef struct Sub_Process Sub_Process;

typedef struct Task Task;

typedef struct Task_Queue Task_Queue;

typedef void *(*thread_callback)(void *args);

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
	/// Mutex for thread_count and active_threads, and keep_alive.
	pthread_mutex_t *thread_count_change;
};

/// Global (Static) Thread Pool struct, feels like it's unavoidable.
extern Thread_Pool *tp;

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
};


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

/// Creates thread pool with the static number of threads.
int Thread_Pool_Init(size_t number_of_threads);

/// Add a task for the thread pool to process, returning a result struct.
Result *Thread_Pool_Add_Task(thread_callback cb, void *args);

/// Will destroy the Result and set it's reference to NULL.
int Thread_Pool_Result_Destroy(Result *result);

/// Will block until result is ready. 
void *Thread_Pool_Obtain_Result(Result *result);

/// Will block until the task queue is empty.
void Thread_Pool_Wait(void);

/// Will pause all threads until a signal to resume to sent.
int Thread_Pool_Pause(void);

/// Will pause all threads until a signal to resume is sent or the time passed has ellapsed.
int Thread_Pool_Timed_Pause(unsigned int seconds);

/// Will resume all currently paused threads.
int Thread_Pool_Resume(void);
#endif /* END THREAD_POOL_H */