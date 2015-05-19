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
			   mutex = malloc(sizeof(pthread_mutex_t));\
			   pthread_mutex_init(mutex, attr); \
			} while(0)
/// Used to quickly initialize a condition variable.
#define INIT_COND(cond, attr) \
			do { \
			   cond = malloc(sizeof(pthread_cond_t));\
			   pthread_cond_init(cond, attr); \
			} while(0)

typedef struct Thread_Pool Thread_Pool;

typedef struct Result Result;

typedef struct Sub_Process Sub_Process;

typedef struct Task Task;

typedef struct Binary_Semaphore Binary_Semaphore;

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

struct Binary_Semaphore {
	/// The lock to simulate a binary semaphore.
	pthread_mutex_t *mutex;
	/// Condition variable to wait on.
	pthread_cond_t *cond;
	/// Dictates whether or not it is held.
	unsigned char held;
};

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
	/// Binary Semaphore for allowing threads to get the next task.
	Binary_Semaphore *semaphore;
	/// Condition variable to signal when a task is added to the queue.
	pthread_cond_t *new_task;
	/// Lock to use on the condition variable
	pthread_mutex_t *getting_task;

};

/// Unlocks the given semaphore.
void BS_Unlock(Binary_Semaphore *semaphore);

/// Locks the given binary semaphore.
void BS_Lock(Binary_Semaphore *semaphore);

/// Used for the threadpool.
Binary_Semaphore *Binary_Semaphore_Create(void);

/// Creates thread pool with the static number of threads.
Thread_Pool *TP_Create(size_t number_of_threads, int parameters);

/// Add a task for the thread pool to process, returning a result struct.
Result *TP_Add_Task(Thread_Pool *tp, thread_callback cb, void *args);

/// Will destroy the Result and set it's reference to NULL.
int TP_Result_Destroy(Result *result);

/// Will block until result is ready. 
void *TP_Obtain_Result(Result *result);
#endif /* END THREAD_POOL_H */