#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "Thread_Pool.h"


void BS_Unlock(Binary_Semaphore *semaphore){
	pthread_mutex_lock(semaphore->mutex);
	// If the thread somehow attempts to unlock without having the actual lock, something went wrong.
	assert(semaphore->held == 1);
	semaphore->held = 0;
	// Signal that the semaphore is no longer being held.
	pthread_cond_signal(semaphore->cond);
	pthread_mutex_unlock(semaphore->mutex);
}

void BS_Lock(Binary_Semaphore *semaphore){
	pthread_mutex_lock(semaphore->mutex);
	while(semaphore->held) pthread_cond_wait(semaphore->cond, semaphore->mutex);
	semaphore->held = 1;
	pthread_mutex_unlock(semaphore->mutex);
}

Binary_Semaphore *Binary_Semaphore_Create(void){
	Binary_Semaphore *semaphore = malloc(sizeof(Binary_Semaphore));
	pthread_mutex_init(semaphore->lock, NULL);
	pthread_cond_init(semaphore->cond, NULL);
	semaphore->held = 0;
	semaphore->lock = BS_Lock;
	semaphore->unlock = BS_Unlock;
	return semaphore;
}

static void *Get_Tasks(void *args){
	Thread_Pool *tp = args;
	while(tp->keep_alive){
		// If queue size is 0.
		if(!tp->queue->size){
			// Acquire the lock to successfully condition wait.
		}
	}

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

static void *Process_Result(Sub_Process *process){
	// Acquire lock to prevent main thread from getting result until ready.
	pthread_mutex_lock(process->result->lock);
	process->result->item = process->task->cb(process->task->args);
	process->result->ready = 1;
	// Signal that the result is ready.
	pthread_cond_signal(process->result->cond);
	// Release lock.
	pthread_mutex_unlock(process->result->lock);
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
	pthread_mutex_lock(result->lock);
	// If the result isn't ready after obtaining the lock, then it must have (somehow) obtained the lock before 
	// the worker thread finished, so release the lock until signaled.
	while(!result->ready) pthread_cond_wait(result->cond, result->lock);
	// Now that it's finished, release the lock.
	pthread_mutex_unlock(result->lock);
	// Since the item is fully processed, return it's item.
	return result->item;
}
