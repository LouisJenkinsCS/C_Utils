#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "Thread_Pool.h"


/* Begin Static, Private functions */

static void Process_Task(Task *task){
	// Acquire lock to prevent main thread from getting result until ready.
	LOCK(task->result->not_ready);
	task->result->item = task->cb(task->args);
	task->result->ready = 1;
	// Signal that the result is ready.
	SIGNAL(task->result->is_ready);
	// Release lock.
	UNLOCK(task->result->not_ready);
	UNLOCK(task->being_processed);
	free(task);
}

static Task *next_task(Task_Queue *queue){
	Task *task = NULL;
	// Note: Generally the head of the queue will always be the one which needs to
	// be processed next, but this is extra insurance.
	for(task = queue->head; task; task = task->next){
		// If the lock can be acquired, then it's not currently being processed.
		// Also, since this is already being processed, remove it from the queue.
		if(TRYLOCK(task->being_processed) == 0){
			queue->head = task->next;
			queue->size--;
			return task;
		}
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
		INCREMENT(tp->active_threads, tp->thread_count_change);
		Process_Task(task);
		DECREMENT(tp->active_threads, tp->thread_count_change);
	}
	DECREMENT(tp->thread_count, tp->thread_count_change);
	return NULL;
}

/* End Static, Private functions. */

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
	INIT_MUTEX(semaphore->mutex, NULL);
	INIT_COND(semaphore->cond, NULL);
	semaphore->held = 0;
	return semaphore;
}

Thread_Pool *TP_Create(size_t number_of_threads, int parameters){
	Thread_Pool *tp = malloc(sizeof(Thread_Pool));
	tp->keep_alive = 1;
	tp->thread_count = tp->active_threads = 0;
	Task_Queue *queue = malloc(sizeof(Task_Queue));
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	//queue->semaphore = Binary_Semaphore_Create();
	INIT_COND(queue->new_task, NULL);
	INIT_MUTEX(queue->getting_task, NULL);
	tp->queue = queue;
	INIT_MUTEX(tp->thread_count_change, NULL);
	int i = 0;
	tp->threads = malloc(sizeof(pthread_t *) * number_of_threads);
	for(;i < number_of_threads; i++){
		tp->threads[i] = malloc(sizeof(pthread_t));
		pthread_create(tp->threads[i], NULL, Get_Tasks, tp);
		tp->thread_count++;
	}
	return tp;
}



Result *TP_Add_Task(Thread_Pool *tp, thread_callback cb, void *args){
	// Initialize Result to be returned.
	Result *result = malloc(sizeof(Result));
	result->ready = 0;
	result->item = NULL;
	INIT_MUTEX(result->not_ready, NULL);
	INIT_COND(result->is_ready, NULL);
	// Initialize Task to be processed.
	Task *task = malloc(sizeof(Task));
	task->cb = cb;
	task->args = args;
	task->next = tp->queue->head;
	tp->queue->head = task;
	INIT_MUTEX(task->being_processed, NULL);
	task->result = result;
	tp->queue->size++;
	SIGNAL(tp->queue->new_task);
	return result;
}

/// Will destroy the Result and set it's reference to NULL.
int TP_Result_Destroy(Result *result){
	pthread_mutex_destroy(result->not_ready);
	pthread_cond_destroy(result->is_ready);
	free(result);
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
