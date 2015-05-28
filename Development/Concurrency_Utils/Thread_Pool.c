#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include "Thread_Pool.h"

/* Define helper macros here. */

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
#define TP_DEBUG 1
/// Print a message if and only if TP_DEBUG is enabled.
#define TP_DEBUG_PRINT(str) (TP_DEBUG ? printf(str) : TP_DEBUG)
/// Print a formatted message if and only if TP_DEBUG is enabled.
#define TP_DEBUG_PRINTF(str, ...)(TP_DEBUG ? printf(str, __VA_ARGS__) : TP_DEBUG)

/* End definition of helper macros. */

/* Initialize Thread Pool as static. */
static Thread_Pool *tp = NULL;

/// Constant to determine how long the queue timeout should be.
static const unsigned int queue_timeout = 5;

/* Begin Static, Private functions */

/// Return the Worker associated with the calling thread.
static Worker *Get_Self(void){
	pthread_t self = pthread_self();
	int i = 0;
	for(;i<tp->thread_count;i++) if(pthread_equal(*(tp->worker_threads[i]->thread), self) return tp->worker_threads[i];
	return NULL;
}

/// Signal handler for pausing all threads.
static void Pause_Handler(){
	if(!tp->paused) return;
	Worker *self = Get_Self();
	assert(self);
	if(self && self->task && self->task->no_pause && !self->task->delayed_pause){ 
		self->task->delayed_pause = 1;
		return;
	}
	pthread_mutex_lock(tp->pause);
	if(tp->seconds_to_pause){
		struct timespec timeout;
		clock_gettime(CLOCK_REALTIME, &timeout);
		timeout.tv_sec += tp->seconds_to_pause;
		tp->seconds_to_pause = 0; 
		while(tp->paused){ 
			int retval = pthread_cond_timedwait(tp->resume, tp->pause, &timeout);
			if(retval == ETIMEDOUT){
				tp->paused = 0;
			}
		}
	} else while(tp->paused) pthread_cond_wait(tp->resume, tp->pause);
	pthread_mutex_unlock(tp->pause);
}

/// Processes a task, store it's result, signals that the result is ready, then destroys the task.
static void Process_Task(Worker *self){
	Task *task = self->task;
	if(task->result){
		pthread_mutex_lock(task->result->not_ready);
		task->result->item = task->callback(task->args);
		task->result->ready = 1;
		pthread_cond_signal(task->result->is_ready);
		pthread_mutex_unlock(task->result->not_ready);
	}
	else task->callback(task->args);
	if(task->delayed_pause) pthread_kill(pthread_self(), SIGUSR1);
	self->task = NULL;
	free(task);
}

static void signal_if_queue_finished(void){
	pthread_mutex_lock(tp->no_tasks);
	if(PBQueue_is_empty(tp->queue) && tp->active_threads == 0) pthread_cond_signal(tp->all_tasks_finished);
	pthread_mutex_unlock(tp->no_tasks);
}

/// The main thread loop to obtain tasks from the task queue.
static void *Get_Tasks(void *args){
	Worker *self = args;
	// Set up the signal handler to pause.
	struct sigaction pause_signal;
	pause_signal.sa_handler = Pause_Handler;
	pause_signal.sa_flags = SA_RESTART;
	sigemptyset(&pause_signal.sa_mask);
	if(sigaction(SIGUSR1, &pause_signal, NULL) == -1) fprintf(stderr, "Get_Task callback was unable to initialize a pause_handler\n");
	while(tp->keep_alive){
		while(tp->keep_alive && !self->task) self->task = PBQueue_Timed_Enqueue(tp->queue, queue_timeout);
		if(!tp->keep_alive) break;
		INCREMENT(tp->active_threads, tp->thread_count_change);
		Process_Task(task);
		DECREMENT(tp->active_threads, tp->thread_count_change);
		signal_if_queue_finished();
	}
	DECREMENT(tp->thread_count, tp->thread_count_change);
	return NULL;
}

/// Determines whether or not the flag has been passed.
static int is_selected(int flag, int mask){
	return (flag & mask)
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

/// Is used to obtain the priority from the flag and set the task's priority to it. Has to be done this way to allow for bitwise.
static void Set_Task_Priority(Task *task, int flags){
	if(is_selected(flags, TP_LOWEST_PRIORITY)) task->priority = TP_LOWEST;
	else if(is_selected(flags, TP_LOW_PRIORITY)) task->priority = TP_LOW;
	else if(is_selected(flags, TP_HIGH_PRIORITY)) task->priority = TP_HIGH;
	else if(is_selected(flags, TP_HIGHEST_PRIORITY)) task->priority = TP_HIGHEST;
	else task->priority = TP_MEDIUM;

}

static int compare_task_priority(void *task_one, void *task_two){
	return ((Task *)task_one)->priority - ((Task *)task_two)->priority;
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
	temp_tp->queue = PBQueue_Create_Unbounded(compare_task_priority);
	INIT_COND(temp_tp->resume, NULL);
	INIT_MUTEX(temp_tp->thread_count_change, NULL);
	INIT_MUTEX(temp_tp->pause, NULL);
	// TODO: Initialize mutex and condition variables in thread pool recently added.
	int i = 0;
	temp_tp->worker_threads = malloc(sizeof(pthread_t *) * number_of_threads);
	tp = temp_tp;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	for(;i < number_of_threads; i++){
		Worker *worker = NULL;
		// TODO: Assign the worker as the argument.
		INIT_WORKER(worker, &attr, Get_Tasks, NULL, i+1);
		tp->worker_threads[i] = worker;
		tp->thread_count++;
	}
	pthread_attr_destroy(&attr);
	return 1;
}



Result *Thread_Pool_Add_Task(thread_callback callback, void *args, int flags){
	if(!tp) return NULL;
	Result *result = NULL;
	if(!is_selected(flags, TP_NO_RESULT)){
		Result *result = malloc(sizeof(Result));
		result->ready = 0;
		result->item = NULL;
		INIT_MUTEX(result->not_ready, NULL);
		INIT_COND(result->is_ready, NULL);
	};
	Task *task = malloc(sizeof(Task));
	task->callback = callback;
	task->args = args;
	Set_Task_Priority(task, flags);
	task->no_pause = is_selected(flags, TP_NO_PAUSE) ? 1 : 0;
	task->result = result;
	PBQueue_Enqueue(tp->queue, task);
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
	pthread_mutex_lock(result->not_ready);
	// If the result isn't ready after obtaining the lock, then it must have (somehow) obtained the lock before 
	// the worker thread finished, so release the lock until signaled.
	while(!result->ready) pthread_cond_wait(result->is_ready, result->not_ready);
	// Now that it's finished, release the lock.
	pthread_mutex_unlock(result->not_ready);
	// Since the item is fully processed, return it's item.
	return result->item;
}

/// Will block until result is ready or time ellapses.
void *Thread_Pool_Timed_Obtain_Result(Result *result, unsigned int seconds);

/// Will block until all tasks are finished.
int Thread_Pool_Wait(void){
	if(!tp) return 0;
	pthread_mutex_lock(tp->no_tasks);
	while(!PBQueue_Is_Empty(tp->queue) || tp->active_threads != 0) pthread_cond_wait(tp->queue->is_finished, tp->queue->no_tasks);
	pthread_mutex_unlock(tp->no_tasks);
	return 1;
}

/// Will block until all tasks are finished or time ellapses.
int Thread_Pool_Timed_Wait(unsigned int seconds){
	if(!tp) return 0;
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += seconds;
	pthread_mutex_lock(tp->no_tasks);
	while(!PBQueue_Is_Empty(tp->queue) || tp->active_threads != 0) {
		int retval = pthread_cond_timedwait(tp->queue->is_finished, tp->queue->no_tasks, &timeout);
		if(retval == ETIMEDOUT) return 0;
	}
	pthread_mutex_unlock(tp->no_tasks);
	return 1;
}

int Thread_Pool_Destroy(void){
	if(!tp) return;
	size_t thread_count = tp->thread_count;
	tp->keep_alive = 0;
	Thread_Pool_Wait();
	// We wait until all tasks are finished before freeing the Thread Pool and threads.
	while(tp->thread_count != 0) pthread_yield();
	// Free all Task_Queue mutexes and condition variables.
	DESTROY_MUTEX(tp->queue->no_tasks);
	DESTROY_MUTEX(tp->queue->await_task);
	DESTROY_MUTEX(tp->queue->adding_task);
	DESTROY_COND(tp->queue->new_task);
	DESTROY_COND(tp->queue->is_finished);
	PBQueue_Destroy(tp->queue);
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
	if(!tp || tp->paused) return 0;
	int successful_pauses = 0;
	int i = 0;
	for(;i<tp->thread_count;i++) if (pthread_kill(*tp->worker_threads[i]->thread, SIGUSR1) == 0) successful_pauses++;
	// The only time this function is successful is if it successfully paused all threads.
	tp->paused = 1;
	return successful_pauses == tp->thread_count;
}

int Thread_Pool_Timed_Pause(unsigned int seconds){
	if(!tp || tp->paused) return 0;
	tp->seconds_to_pause = seconds;
	return Thread_Pool_Pause();
}

int Thread_Pool_Resume(void){
	if(!tp || !tp->paused) return 0;
	tp->paused = 0;
	int result = pthread_cond_broadcast(tp->resume) == 0;	
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