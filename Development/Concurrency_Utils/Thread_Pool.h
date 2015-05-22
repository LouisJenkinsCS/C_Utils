#ifndef THREAD_POOL_H
#define THREAD_POOL_H


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
	/// Flag used to pause all threads
	volatile unsigned char paused;
	/// Mutex for thread_count and active_threads, and keep_alive.
	pthread_mutex_t *thread_count_change;
	/// Condition variable used to resume all threads.
	pthread_cond_t *resume;
	/// Mutex accompanied by the condition variable.
	pthread_mutex_t *pause;
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

/// Destroy the thread pool.
int Thread_Pool_Destroy(void);

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