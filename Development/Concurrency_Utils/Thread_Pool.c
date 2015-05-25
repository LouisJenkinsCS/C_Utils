#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "Thread_Pool.h"

/* Define helper macros here. */

/// Used to determine if a certain flag was passed.
#define SELECTED(argument, flag) ((argument & flag))
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
/// Used to quickly initialize a worker thread.
#define INIT_WORKER(worker, attribute, callback, args, id) \
			do { \
				worker = malloc(sizeof(Worker)); \
				worker->thread = malloc(sizeof(pthread_t)); \
				pthread_create(worker->thread, attribute, callback, args); \
				worker->thread_id = id; \
				worker->task = NULL; \
			} while(0)
/// Frees the worker thread.
#define DESTROY_WORKER(worker) \
			do { \
				free(worker->thread); \
				free(worker); \
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

/* End definition of helper macros. */

/* Initialize Thread Pool as static. */
static Thread_Pool *tp = NULL;

/* Begin Static, Private functions */

/// Return the Worker associated with the calling thread.
static Worker *Get_Self(void){
	int i = 0;
	for(;i<tp->thread_count;i++) if(pthread_equal(*(tp->worker_threads[i]->thread), pthread_self())) return tp->worker_threads[i];
	return NULL;
}


/// The callback used to handle pausing all threads.
// Also this is the primary reason for using a global (static) thread pool.
static void Pause_Handler(){
	Worker *self = Get_Self();
	if(self && self->task && self->task->preference == NO_PAUSE && self->task->status != DELAYED_PAUSE){ 
		self->task->status = DELAYED_PAUSE;
		return;
	}
	LOCK(tp->pause);
	while(tp->paused) WAIT(tp->resume, tp->pause);
	UNLOCK(tp->pause);
}

static void Add_Task_As_Head(Task *task){
	task->next = tp->queue->head;
	tp->queue->head = task;
}

static void Add_Task_As_Tail(Task *task){
	tp->queue->tail->next = tp->queue->tail = task;
	task->next = NULL;
}

static void Add_Task_As_Only(Task *task){
	task->next = NULL;
	tp->queue->head = tp->queue->tail = task;
}

static void Add_Task_After(Task *task, Task *previous_task){
	task->next = previous_task->next;
	previous_task->next = task;
}

static void Add_Task_Sorted(Task *task){
	LOCK(tp->queue->adding_task);
	if(tp->queue->size == 0) Add_Task_As_Only(task); 
	else if(tp->queue->size == 1){
		if(task->priority > tp->queue->head->priority) Add_Task_As_Head(task);
		else Add_Task_As_Tail(task);
	} else if (task->priority == TP_LOWEST) Add_Task_As_Tail(task);
	else {
		Task *task_to_compare = NULL;
		// To avoid adding a doubly linked list, I keep track of the previous task.
		Task *previous_task = NULL;
		for(previous_task = task_to_compare = tp->queue->head; task_to_compare; previous_task = task_to_compare, task_to_compare = task_to_compare->next){
			if(task->priority > task_to_compare->priority) Add_Task_After(task, previous_task);
			else if (!task->next) Add_Task_As_Tail(task);
		}
	}
	tp->queue->size++;
	UNLOCK(tp->queue->adding_task);
}

/// Processes a task, store it's result, signals that the result is ready, then destroys the task.
static void Process_Task(Task *task){
	// Acquire lock to prevent main thread from getting result until ready.
	if(tp->paused)PAUSE(pthread_self());
	if(task->result){
		LOCK(task->result->not_ready);
		task->result->item = task->callback(task->args);
		task->result->ready = 1;
		SIGNAL(task->result->is_ready);
		UNLOCK(task->result->not_ready);
	}
	else task->callback(task->args);
	UNLOCK(task->being_processed);	
	DESTROY_MUTEX(task->being_processed);
	if(task->preference == NO_PAUSE && tp->paused) PAUSE(pthread_self());
	Get_Self()->task = NULL;
	free(task);
	TP_DEBUG_PRINT("A thread finished their task!\n");
}

/// Obtains the next task from the queue.
static Task *next_task(Task_Queue *queue){
	Task *task = NULL;
	// Note: Generally the head of the queue will always be the one which needs to
	// be processed next, but this is extra insurance.
	LOCK(queue->adding_task);
	for(task = queue->head; task; task = task->next){
		// If the lock can be acquired, then it's not currently being processed.
		// Also, since this is already being processed, remove it from the queue.
		if(TRYLOCK(task->being_processed) == 0){
			queue->head = task->next;
			queue->size--;
			UNLOCK(queue->adding_task);
			Get_Self()->task = task;
			return task;
		}
	}
	TP_DEBUG_PRINT("Task returned NULL\n");
	UNLOCK(queue->adding_task);
	return NULL;
}

/// The main thread loop to obtain tasks from the task queue.
static void *Get_Tasks(void *args){
	// Set up the signal handler to pause.
	struct sigaction pause_signal;
	pause_signal.sa_handler = Pause_Handler;
	pause_signal.sa_flags = SA_RESTART;
	sigemptyset(&pause_signal.sa_mask);
	if(sigaction(SIGUSR1, &pause_signal, NULL) == -1) fprintf(stderr, "Get_Task callback was unable to initialize a pause_handler\n");
	while(tp->keep_alive){
		// Wait until signal is sent, when task is given.
		LOCK(tp->queue->await_task);
		while(tp->queue->size == 0 && tp->keep_alive){
			TP_DEBUG_PRINTF("Queue Size: %d, Keep_Alive: %d\n", tp->queue->size, tp->keep_alive);
			// If the queue size is empty, wait until something has been added.
			WAIT(tp->queue->new_task, tp->queue->await_task);
		}
		// If while it was waiting, the keep_alive flag has changed, then break the while loop.
		if(!tp->keep_alive) {
			UNLOCK(tp->queue->await_task);
			SIGNAL(tp->queue->new_task);
			break;
		}
		Task *task = next_task(tp->queue);
		//printf("A thread got a task!\n");
		UNLOCK(tp->queue->await_task);
		//printf("A thread unlocked: await_task\n");
		if(!task) continue;
		INCREMENT(tp->active_threads, tp->thread_count_change);
		Process_Task(task);
		DECREMENT(tp->active_threads, tp->thread_count_change);
		LOCK(tp->queue->no_tasks);
		// Note to self: This is probably the cause for the random results for completed iterations.
		// If all 8 threads, while extremely rare but very much possible, are waiting on this lock,
		// after they have decremented the active_thread count and the task queue is 0, it will be
		// flagged as finish when it actually isn't.
		if(tp->queue->size == 0 && tp->active_threads == 0) SIGNAL(tp->queue->is_finished);
		UNLOCK(tp->queue->no_tasks);
	}
	DECREMENT(tp->thread_count, tp->thread_count_change);
	TP_DEBUG_PRINTF("Thread count decremented: %d\n", tp->thread_count);
	return NULL;
}

static void Debug_Print_Thread_Pool(void){
	printf("Thread Pool:\n\n\n");
	int i = 0;
	for(;i<tp->thread_count;i++){
		Worker *worker = tp->worker_threads[i];
		printf("Worker Thread %d:\nThread Alive? %d\nThread_ID: %d\nHas Task?: %d\n\n",
			i, worker->thread ? 1 : 0, worker->thread_id, worker->task ? 1 : 0);
	}
}

static void *Monitor_Threads(void *args){
	// TODO: Implement a method to monitor all tasks and threads. If a task is waiting for over 60 seconds, then
	// spawn a new thread. If the thread pool has been empty for longer than 60 seconds, despawn a thread until
	// the minimum amount of threads are alive.
}

/// Is used to obtain the priority from the flag and set the task's priority to it. Has to be done this way to allow for bitwise.
static void Set_Task_Priority(Task *task, int flags){
	if(SELECTED(flags, TP_LOWEST_PRIORITY)) task->priority = TP_LOWEST;
	else if(SELECTED(flags, TP_LOW_PRIORITY)) task->priority = TP_LOW;
	else if(SELECTED(flags, TP_HIGH_PRIORITY)) task->priority = TP_HIGH;
	else if(SELECTED(flags, TP_HIGHEST_PRIORITY)) task->priority = TP_HIGHEST;
	else task->priority = TP_MEDIUM;

}

/// Used to obtain the pause preference flag from the parameter to the task's preference.
static void Set_Task_Preference(Task *task, int flags){
	if(SELECTED(flags, TP_NO_PAUSE)) task->preference = NO_PAUSE;
	else task->preference = PAUSE;
}

/* End Static, Private functions. */

int Thread_Pool_Init(size_t number_of_threads){
	// If the thread pool is already initialized, return.
	if(tp) return 0; 
	// Instead of directly allocating the static thread pool, we make a temporary one and make the static thread pool point to it later.
	// The reason is because once the thread pool is allocated, it opens up the opportunity for undefined behavior since it no longer is NULL.
	Thread_Pool *temp_tp = malloc(sizeof(Thread_Pool));
	temp_tp->keep_alive = 1;
	temp_tp->paused = 0;
	temp_tp->thread_count = temp_tp->active_threads = 0;
	Task_Queue *queue = malloc(sizeof(Task_Queue));
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	INIT_COND(queue->new_task, NULL);
	INIT_COND(queue->is_finished, NULL);
	INIT_MUTEX(queue->await_task, NULL);
	INIT_MUTEX(queue->adding_task, NULL);
	INIT_MUTEX(queue->no_tasks, NULL);
	temp_tp->queue = queue;
	INIT_COND(temp_tp->resume, NULL);
	INIT_MUTEX(temp_tp->thread_count_change, NULL);
	INIT_MUTEX(temp_tp->pause, NULL);
	int i = 0;
	temp_tp->worker_threads = malloc(sizeof(pthread_t *) * number_of_threads);
	// This is the latest I can actually assign the static thread pool, as once the worker threads are created, they will attempt
	// to dereference the thread pool anyway to see if it's keep_alive is still flagged. So besides overcomplicating it further, I
	// decide to just leave it at this. The chance of it messing up is minimized.
	tp = temp_tp;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	for(;i < number_of_threads; i++){
		Worker *worker = temp_tp->worker_threads[i];
		INIT_WORKER(worker, &attr, Get_Tasks, NULL, i+1);
		temp_tp->thread_count++;
	}
	pthread_attr_destroy(&attr);
	Debug_Print_Thread_Pool();
	return 1;
}



Result *Thread_Pool_Add_Task(thread_callback callback, void *args, int flags){
	if(!tp) return;
	// Initialize Result to be returned.
	// TODO: Get flag from flags to determine whether to return a Result or NULL.
	Result *result = NULL;
	if(!SELECTED(flags, TP_NO_RESULT)){
		Result *result = malloc(sizeof(Result));
		result->ready = 0;
		result->item = NULL;
		INIT_MUTEX(result->not_ready, NULL);
		INIT_COND(result->is_ready, NULL);
	};
	// Initialize Task to be processed.
	Task *task = malloc(sizeof(Task));
	task->callback = callback;
	task->args = args;
	Set_Task_Priority(task, flags);
	Set_Task_Preference(task, flags);
;	task->status = WAITING;
	task->time_added = malloc(sizeof(time_t));
	time(task->time_added);
	Add_Task_Sorted(task);
	INIT_MUTEX(task->being_processed, NULL);
	task->result = result;
	tp->queue->size++;
	SIGNAL(tp->queue->new_task);
	return result;
}

/// Will destroy the Result and set it's reference to NULL.
int Thread_Pool_Result_Destroy(Result *result){
	DESTROY_MUTEX(result->not_ready);
	DESTROY_COND(result->is_ready);
	free(result);
	return 1;
}
/// Will block until result is ready. 
void *Thread_Pool_Obtain_Result(Result *result){
	// If there is no thread pool, return.
	if(!tp) return NULL;
	// Attempts to obtain the lock before proceeding, since the worker thread will only unlock when it's finished.
	LOCK(result->not_ready);
	// If the result isn't ready after obtaining the lock, then it must have (somehow) obtained the lock before 
	// the worker thread finished, so release the lock until signaled.
	while(!result->ready) WAIT(result->is_ready, result->not_ready);
	// Now that it's finished, release the lock.
	UNLOCK(result->not_ready);
	// Since the item is fully processed, return it's item.
	return result->item;
}

void Thread_Pool_Wait(void){
	if(!tp) return;
	LOCK(tp->queue->no_tasks);
	while(tp->queue->size != 0 || tp->active_threads != 0) WAIT(tp->queue->is_finished, tp->queue->no_tasks);
	UNLOCK(tp->queue->no_tasks);
}

int Thread_Pool_Destroy(void){
	if(!tp) return;
	size_t thread_count = tp->thread_count;
	tp->keep_alive = 0;
	// Broadcast to all threads to wake up on new_task condition variable.
	BROADCAST(tp->queue->new_task);
	Thread_Pool_Wait();
	// We wait until all tasks are finished before freeing the Thread Pool and threads.
	TP_DEBUG_PRINTF("Queue Size: %d\n", tp->queue->size);
	while(tp->thread_count != 0) pthread_yield();
	// Free all Task_Queue mutexes and condition variables.
	DESTROY_MUTEX(tp->queue->no_tasks);
	DESTROY_MUTEX(tp->queue->await_task);
	DESTROY_MUTEX(tp->queue->adding_task);
	DESTROY_COND(tp->queue->new_task);
	DESTROY_COND(tp->queue->is_finished);
	free(tp->queue);
	DESTROY_MUTEX(tp->thread_count_change);
	DESTROY_COND(tp->resume);
	DESTROY_MUTEX(tp->pause);
	TP_DEBUG_PRINTF("Thread_Count size: %d\n", tp->thread_count);
	int i = 0;
	for(;i<thread_count;i++) DESTROY_WORKER(tp->worker_threads[i]);
	free(tp->worker_threads);
	free(tp);
	tp = NULL;
	return 1;
}

int Thread_Pool_Pause(void){
	if(!tp) return 0;
	int successful_pauses = 0;
	int i = 0;
	for(;i<tp->thread_count;i++) if (PAUSE(*tp->worker_threads[i]->thread) == 0) successful_pauses++;
	// The only time this function is successful is if it successfully paused all threads.
	tp->paused = 1;
	return successful_pauses == tp->thread_count;
}

int Thread_Pool_Timed_Pause(unsigned int seconds);

int Thread_Pool_Resume(void){
	if(!tp || !tp->paused) return 0;
	int result = BROADCAST(tp->resume) == 0;
	tp->paused = 0;
	return result;
}

/* Undefine all user macros below. */

#undef LOCK
#undef UNLOCK
#undef TRYLOCK
#undef WAIT
#undef SIGNAL
#undef BROADCAST
#undef PAUSE
#undef INCREMENT
#undef DECREMENT
#undef INIT_MUTEX
#undef INIT_COND
#undef INIT_WORKER
#undef DESTROY_WORKER
#undef DESTROY_MUTEX
#undef DESTROY_COND
#undef TP_DEBUG
#undef TP_DEBUG_PRINT
#undef TP_DEBUG_PRINTF