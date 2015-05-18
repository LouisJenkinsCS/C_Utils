#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "Thread_Pool.h"

/// Used to lock the given mutex.
#define LOCK(mutex) pthread_mutex_lock(mutex);
/// Used to try to lock the mutex, returning immediately on failure.
#define TRYLOCK(mutex) pthread_mutex_trylock(mutex);
/// Used to unlock the given mutex.
#define UNLOCK(mutex) pthread_mutex_unlock(mutex);
/// Causes the current thread to wait for a signal to be sent.
#define WAIT(condition, mutex) pthread_cond_wait(condition, mutex);
/// Signals a thread waiting on the condition based on a default scheduler.
#define SIGNAL(condition) pthread_cond_signal(condition);
/// Used to broadcast to all threads waiting on the condition variable.
#define BROADCAST(condition) pthread_cond_broadcast(condition);
/// Used to atomically increment the count.
#define INCREMENT(var, mutex) (do { \
								LOCK(mutex); \
								var++; \
								UNLOCK(mutex); \
								})
/// Used to atomically decrement the count.
#define DECREMENT(var, mutex) (do { \
								LOCK(mutex); \
								var--; \
								UNLOCK(mutex); \
							    })
void BS_Unlock(Binary_Semaphore *semaphore){
	LOCK(semaphore->mutex);
	// If the thread somehow attempts to unlock without having the actual lock, something went wrong.
	assert(semaphore->held == 1);
	semaphore->held = 0;
	// Signal that the semaphore is no longer being held.
	SIGNAL(semaphore->cond);
	UNLOCK(semaphore->mutex);
}

void BS_Lock(Binary_Semaphore *semaphore){
	LOCK(semaphore->mutex);
	while(semaphore->held) WAIT(semaphore->cond, semaphore->mutex);
	semaphore->held = 1;
	UNLOCK(semaphore->mutex);
}

Binary_Semaphore *Binary_Semaphore_Create(void){
	Binary_Semaphore *semaphore = malloc(sizeof(Binary_Semaphore));
	pthread_mutex_init(semaphore->mutex, NULL);
	pthread_cond_init(semaphore->cond, NULL);
	semaphore->held = 0;
	semaphore->lock = BS_Lock;
	semaphore->unlock = BS_Unlock;
	return semaphore;
}

static Task *next_task(Task_Queue *queue){
	Task *task = NULL;
	for(task = queue->head; task; task = task->next){
		// int pthread_mutex_trylock(pthread_mutex_t *mutex) returns 0 on success.
		if(!TRYLOCK(task->being_processed)) return task;
	}
	return NULL;
}

static void *Get_Tasks(void *args){
	Thread_Pool *tp = args;
	while(tp->keep_alive){
		// Wait until signal is sent, when task is given.
		LOCK(tp->queue->getting_task);
		while(!tp->queue->size){
			// If the queue size is empty, wait until something has been added.
			WAIT(tp->queue->new_task, tp->queue->getting_task);
		}
		// If while it was waiting, the keep_alive flag has changed, then break the while loop.
		if(!tp->keep_alive) break;
		Task *task = next_task(tp->queue);
		UNLOCK(tp->queue->getting_task);
		if(!task) continue;
		INCREMENT(tp->active_threads, thread_count_change);
		Process_Task(task);
		DECREMENT(tp->active_threads, thread_count_change);
	}
	DECREMENT(tp->thread_count, tp->thread_count_change);
	return NULL;
}

Thread_Pool *TP_Create(size_t number_of_threads, int parameters){
	Thread_Pool *tp = malloc(sizeof(Thread_Pool));
	tp->keep_alive = 1;
	tp->thread_count = tp->active_threads = 0;
	Task_Queue *queue = malloc(sizeof(Task_Queue));
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	queue->semaphore = Binary_Semaphore_Create();
	tp->queue = queue;
	int i = 0;
	// Create the number of threads, also sends the threadpool as an argument.
	for(;i < number_of_threads; i++){
		tp->threads[i] = malloc(sizeof(pthread_t));
		pthread_create(tp->threads[i], NULL, Get_Tasks, tp);
	}
}

static void add_task(Sub_Process *process){

}

static void *Process_Task(Task *task){
	// Acquire lock to prevent main thread from getting result until ready.
	LOCK(task->result->not_ready);
	task->result->item = task->cb(task->args);
	task->result->ready = 1;
	// Signal that the result is ready.
	SIGNAL(task->result->is_ready);
	// Release lock.
	UNLOCK(task->result->not_ready);
	UNLOCK(task->being_processed);
	return NULL;
}

Result *TP_Add_Task(thread_pool *tp, thread_callback cb, void *args, int parameters){
	// Initialize Result to be returned.
	Result *result = malloc(sizeof(Result));
	result->ready = 0;
	result->item = NULL;
	// Initialize Task to be processed.
	Task *task = malloc(sizeof(Task));
	task->cb = cb;
	task->args = arg;
	// Initialize Sub_Process to process task and result.
	Sub_Process *proc = malloc(sizeof(Sub_Process));
	proc->result = result;
	proc->task = task;
	// Add task to job queue.
	add_task(proc);
	return result;
}

/// Will destroy the Result and set it's reference to NULL.
int TP_Result_Destroy(Result *result){
	pthread_mutex_destroy(result->lock);
	pthread_cond_destroy(result->cond);
	free(result);
	result = NULL;
	return 1;
}
/// Will block until result is ready. 
void *TP_Obtain_Result(Result *result){
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
