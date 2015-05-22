#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "Thread_Pool.h"

/* Initialize Thread Pool as static. */
static Thread_Pool *tp = NULL;

/* Begin Static, Private functions */

/// The callback used to handle pausing all threads.
/// Also this is the primary reason for using a global (static) thread pool.
static void Pause_Handler(){
	LOCK(tp->pause);
	while(tp->paused) WAIT(tp->resume, tp->pause);
	UNLOCK(tp->pause);
}

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
	DESTROY_MUTEX(task->being_processed);
	free(task);
	TP_DEBUG_PRINT("A thread finished their task!\n");
}

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
			return task;
		}
	}
	TP_DEBUG_PRINT("Task returned NULL\n");
	UNLOCK(queue->adding_task);
	return NULL;
}

static void *Get_Tasks(void *args){
	// Set up the signal handler to pause.
	struct sigaction pause_signal;
	pause_signal.sa_handler = Pause_Handler;
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
	pthread_exit(NULL);
	return NULL;
}

/* End Static, Private functions. */

int Thread_Pool_Init(size_t number_of_threads){
	tp = malloc(sizeof(Thread_Pool));
	tp->keep_alive = 1;
	tp->paused = 0;
	tp->thread_count = tp->active_threads = 0;
	Task_Queue *queue = malloc(sizeof(Task_Queue));
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	INIT_COND(queue->new_task, NULL);
	INIT_COND(queue->is_finished, NULL);
	INIT_MUTEX(queue->await_task, NULL);
	INIT_MUTEX(queue->adding_task, NULL);
	INIT_MUTEX(queue->no_tasks, NULL);
	tp->queue = queue;
	INIT_COND(tp->resume, NULL);
	INIT_MUTEX(tp->thread_count_change, NULL);
	INIT_MUTEX(tp->pause, NULL);
	int i = 0;
	tp->threads = malloc(sizeof(pthread_t *) * number_of_threads);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	for(;i < number_of_threads; i++){
		tp->threads[i] = malloc(sizeof(pthread_t));
		pthread_create(tp->threads[i], &attr, Get_Tasks, NULL);
		tp->thread_count++;
	}
	pthread_attr_destroy(&attr);
	return 1;
}



Result *Thread_Pool_Add_Task(thread_callback cb, void *args){
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
	LOCK(tp->queue->adding_task);
	if(tp->queue->size == 0){
		task->next = NULL;
		tp->queue->head = tp->queue->tail = task;
	} else {
		tp->queue->tail->next = tp->queue->tail = task;
		task->next = NULL;
	}
	UNLOCK(tp->queue->adding_task);
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
	LOCK(tp->queue->no_tasks);
	while(tp->queue->size != 0 || tp->active_threads != 0) WAIT(tp->queue->is_finished, tp->queue->no_tasks);
	UNLOCK(tp->queue->no_tasks);
}

int Thread_Pool_Destroy(void){
	size_t thread_count = tp->thread_count;
	tp->keep_alive = 0;
	// Broadcast to all threads to wake up on new_task condition variable.
	BROADCAST(tp->queue->new_task);
	Thread_Pool_Wait();
	// We wait until all tasks are finished before freeing the Thread Pool and threads.
	TP_DEBUG_PRINTF("Queue Size: %d\n", tp->queue->size);
	while(tp->thread_count != 0) sleep(0);
	TP_DEBUG_PRINT("Made it past Wait!\n");
	// Free all Task_Queue mutexes and condition variables.
	DESTROY_MUTEX(tp->queue->no_tasks);
	TP_DEBUG_PRINT("Destroyed Mutex: no_tasks\n");
	DESTROY_MUTEX(tp->queue->await_task);
	TP_DEBUG_PRINT("Destroyed Mutex: await_task\n");
	DESTROY_MUTEX(tp->queue->adding_task);
	TP_DEBUG_PRINT("Destroyed Mutex: adding_task\n");
	DESTROY_COND(tp->queue->new_task);
	TP_DEBUG_PRINT("Destroyed Condition Variable: new_task\n");
	DESTROY_COND(tp->queue->is_finished);
	TP_DEBUG_PRINT("Destroyed Condition_Variable: is_finished\n");
	free(tp->queue);
	TP_DEBUG_PRINT("Destroyed Task_Queue\n");
	// Free Thread_Pool's mutexes and condition variables.
	DESTROY_MUTEX(tp->thread_count_change);
	TP_DEBUG_PRINT("Destroyed Mutex: thread_count_change\n");
	TP_DEBUG_PRINTF("Thread_Count size: %d\n", tp->thread_count);
	free(tp->threads);
	free(tp);
	tp = NULL;
	return 1;
}

int Thread_Pool_Pause(void){
	int successful_pauses;
	int i = 0;
	for(;i<tp->thread_count;i++) if (PAUSE(*tp->threads[i]) == 0) successful_pauses++;
	// The only time this function is successful is if it successfully paused all threads.
	tp->paused = 1;
	return successful_pauses == tp->thread_count;
}

int Thread_Pool_Timed_Pause(unsigned int seconds);

int Thread_Pool_Resume(void){
	int result = BROADCAST(tp->resume) == 0;
	tp->paused = 0;
	return result;
}