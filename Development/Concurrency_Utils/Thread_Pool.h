typedef struct Thread_Pool Thread_Pool;

typedef struct Result Result;

typedef struct Sub_Process Sub_Process;

typedef struct Task Task;

typedef struct Binary_Semaphore Binary_Semaphore;

typedef struct Task_Queue Task_Queue;

typedef void *(*thread_callback)(void *args);

struct Thread_Pool {
	/// Array of threads.
	pthread_t *threads;
	/// The queue with all jobs assigned to it.
	Task_Queue *queue;
	/// Amount of threads currently created, A.K.A Max amount.
	size_t thread_count;
	/// Amount of threads currently active.
	size_t active_threads;
	/// Flag to keep all threads alive.
	volatile unsigned char keep_alive;
};

struct Binary_Semaphore {
	/// The lock to simulate a binary semaphore.
	pthread_mutex_t *mutex;
	/// Condition variable to wait on.
	pthread_cond_t *cond;
	/// Dictates whether or not it is held.
	unsigned char held;
	/// Function pointer for obtaining the lock.
	void (*lock)(Binary_Semaphore *) lock;
	/// Function pointer for releasing the lock.
	void (*unlock)(Binary_Semaphore *) unlock;
};

struct Result {
	/// Lock to protect contents of 'Result'
	pthread_mutex_t *lock;
	/// Condition variable to signal result being ready.
	pthread_cond_t *cond;
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
	pthread_mutex_t *lock;
};

struct Sub_Process {
	/// Task to be processed.
	Task *task;
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
Result *TP_Add_Task(thread_pool *tp, thread_callback cb, void *args, int parameters);

/// Will destroy the Result and set it's reference to NULL.
int TP_Result_Destroy(Result *result);

/// Will block until result is ready. 
void *TP_Obtain_Result(Result *result);