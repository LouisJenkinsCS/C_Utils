#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <Thread_Pool.h>

static MU_Logger_t *logger = NULL;

#define MU_LOG_SERVER(message, ...) MU_LOG_CUSTOM(logger, "SERVER", message, ##__VA_ARGS__)

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Thread_Pool/Logs/Thread_Pool.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

/* Begin Static, Private functions */

/// Return the Worker associated with the calling thread.
static TP_Worker_t *Get_Self(TP_Pool_t *tp){
	pthread_t self = pthread_self();
	int thread_count = tp->thread_count;
	int i = 0;
	for(;i<thread_count;i++){
		if(pthread_equal(*(tp->worker_threads[i]->thread), self)){
			return tp->worker_threads[i];
		}
	}
	MU_LOG_ERROR(logger, "A worker thread could not be identified from pthread_self!\n");
	return NULL;
}


static void Process_Task(TP_Task_t *task){
	void *retval = task->callback(task->args);
	if(task->result){
		task->result->item = retval;
		MU_Event_signal(task->result->is_ready);
	}
	free(task);
}

/// Determines whether or not the flag has been passed.
static int is_selected(int flag, int mask){
	return (flag & mask);
}

/// The main thread loop to obtain tasks from the task queue.
static void *Get_Tasks(void *args){
	TP_Pool_t *tp = args;
	// keep_alive thread is initialized to 0 meaning it isn't setup, but set to 1 after it is, so it is dual-purpose.
	while(!tp->init_error && !tp->keep_alive){
		pthread_yield();
	}
	if(tp->init_error){
		// If there is an initialization error, we need to abort ASAP as the thread actually gets freed, so we can't risk dereferencing self.
		atomic_fetch_sub(&tp->thread_count, 1);
		pthread_exit(NULL);
	}
	TP_Worker_t *self = Get_Self(tp);
	// Note that this while loop checks for keep_alive to be true, rather than false. 
	while(tp->keep_alive){
		TP_Task_t *task = NULL;
		task = PBQueue_dequeue(tp->queue, -1);
		if(!tp->keep_alive){
			break;
		}
		// Note that the paused event is only waited upon if it the event interally is flagged.
		MU_Event_wait(tp->resume, tp->seconds_to_pause);
		// Also note that if we do wait for a period of time, the thread pool could be set to shut down.
		if(!tp->keep_alive){
			break;
		}
		atomic_fetch_add(&tp->active_threads, 1);
		MU_LOG_VERBOSE(logger, "Thread #%d: received a task!\n", self->thread_id);
		Process_Task(task);
		MU_LOG_VERBOSE(logger, "Thread #%d: finished a task!\n", self->thread_id);
		atomic_fetch_sub(&tp->active_threads, 1);
		// Close as we are going to get testing if the queue is fully empty. 
		if(PBQueue_size(tp->queue) == 0 && atomic_load(&tp->active_threads) == 0){
			MU_Event_signal(tp->finished);
		}
	}
	atomic_fetch_sub(&tp->thread_count, 1);
	MU_LOG_VERBOSE(logger, "Thread #%d: Exited!\n", self->thread_id);
	return NULL;
}

/// Is used to obtain the priority from the flag and set the task's priority to it. Has to be done this way to allow for bitwise.
static void Set_Task_Priority(TP_Task_t *task, int flags){
	if(is_selected(flags, TP_LOWEST_PRIORITY)) task->priority = TP_LOWEST;
	else if(is_selected(flags, TP_LOW_PRIORITY)) task->priority = TP_LOW;
	else if(is_selected(flags, TP_HIGH_PRIORITY)) task->priority = TP_HIGH;
	else if(is_selected(flags, TP_HIGHEST_PRIORITY)) task->priority = TP_HIGHEST;
	else task->priority = TP_MEDIUM;

}

static char *Get_Task_Priority(TP_Task_t *task){
	if(task->priority == TP_LOWEST) return "Lowest";
	else if(task->priority == TP_LOW) return "Low";
	else if(task->priority == TP_MEDIUM) return "Medium";
	else if(task->priority == TP_HIGH) return "High";
	else if(task->priority == TP_HIGHEST) return "Highest";
	else{
		MU_LOG_ERROR(logger, "Was unable to retrieve the priority of the task!\n");
		return NULL;
	}
}

/// Simple comparator to compare two task priorities.
static int compare_task_priority(void *task_one, void *task_two){
	return ((TP_Task_t *)task_one)->priority - ((TP_Task_t *)task_two)->priority;
}

/* End Static, Private functions. */

TP_Pool_t *TP_Pool_create(size_t pool_size){
	size_t workers_allocated = 0;
	TP_Pool_t *tp = calloc(1, sizeof(TP_Pool_t));
	if(!tp){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	tp->thread_count = ATOMIC_VAR_INIT(0);
	tp->active_threads = ATOMIC_VAR_INIT(0);
	tp->queue = PBQueue_create(compare_task_priority, -1);
	tp->resume = MU_Event_create(false, "Paused", logger);
	if(!tp->resume){
		MU_LOG_ERROR(logger, "MU_Event_create: 'Was unable to create event Pause/Resume!'");
		goto error;
	}
	tp->finished = MU_Event_create(false, "Finished", logger);
	if(!tp->finished){
		MU_LOG_ERROR(logger, "MU_Event_create: 'Was unable to create event Finished!'");
		goto error;
	}
	tp->worker_threads = malloc(sizeof(pthread_t *) * pool_size);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)
;	size_t i = 0;
	for(;i < pool_size; i++){
		TP_Worker_t *worker = calloc(1, sizeof(TP_Worker_t));
		if(!worker){
			MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
		workers_allocated++;
		worker->thread = malloc(sizeof(pthread_t));
		if(!worker->thread){
			MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
		worker->thread_id = i+1;
		int create_error = pthread_create(worker->thread, NULL, Get_Tasks, tp);
		if(create_error){
			MU_LOG_ERROR(logger, "pthread_create: '%s'", strerror(create_error));
			goto error;
		}
		tp->worker_threads[i] = worker;
		tp->thread_count++;
	}
	pthread_attr_destroy(&attr);
	tp->keep_alive = 1;
	return tp;

	error:
		if(tp){
			tp->init_error = true;
			MU_Event_destroy(tp->resume);
			MU_Event_destroy(tp->)
		}
		return NULL;
}

TP_Result_t *TP_Pool_add(thread_callback callback, void *args, int flags){
	if(!tp) return NULL;
	Result *result = NULL;
	if(!is_selected(flags, TP_NO_RESULT)){
		result = malloc(sizeof(Result));
		result->ready = 0;
		result->item = NULL;
		init_mutex(&result->not_ready, NULL);
		init_cond(&result->is_ready, NULL);
	};
	Task *task = malloc(sizeof(Task));
	task->callback = callback;
	task->args = args;
	Set_Task_Priority(task, flags);
	task->no_pause = is_selected(flags, TP_NO_PAUSE) ? 1 : 0;
	task->delayed_pause = 0;
	task->result = result;
	PBQueue_Enqueue(tp->queue, task);
	MU_LOG_VERBOSE(logger, "A task of %s priority has been added to the task_queue!\n", Get_Task_Priority(task));
	return result;
}

int Thread_Pool_Clear_Tasks(void){
	MU_LOG_VERBOSE(logger, "Clearing all tasks from Thread Pool!\n");
	return PBQueue_Clear(tp->queue, NULL);
}

/// Will destroy the Result and set it's reference to NULL.
int Thread_Pool_Result_Destroy(Result *result){
	destroy_mutex(result->not_ready);
	destroy_cond(result->is_ready);
	free(result);
	return 1;
}
/// Will block until result is ready. 
void *Thread_Pool_Obtain_Result(Result *result){
	if(!tp || !result) return NULL;
	pthread_mutex_lock(result->not_ready);
	while(!result->ready) pthread_cond_wait(result->is_ready, result->not_ready);
	pthread_mutex_unlock(result->not_ready);
	return result->item;
}

/// Will block until result is ready or time ellapses.
void *Thread_Pool_Timed_Obtain_Result(Result *result, unsigned int seconds){
	if(!tp || !result) return NULL;
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += seconds;
	pthread_mutex_lock(result->not_ready);
	while(!result->ready) {
		int retval = pthread_cond_timedwait(result->is_ready, result->not_ready, &timeout);
		if(retval == ETIMEDOUT) return NULL;
	}
	pthread_mutex_unlock(result->not_ready);
	return result->item;
}

/// Will block until all tasks are finished.
int Thread_Pool_Wait(void){
	if(!tp) return 0;
	pthread_mutex_lock(tp->no_tasks);
	while(!PBQueue_Is_Empty(tp->queue) || tp->active_threads != 0) pthread_cond_wait(tp->all_tasks_finished, tp->no_tasks);
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
		int retval = pthread_cond_timedwait(tp->all_tasks_finished, tp->no_tasks, &timeout);
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
	/*while(tp->thread_count != 0) {
		sleep(1);
	}*/
	destroy_mutex(tp->no_tasks);
	destroy_cond(tp->all_tasks_finished);
	PBQueue_Destroy(tp->queue);
	destroy_mutex(tp->thread_count_change);
	destroy_cond(tp->resume);
	destroy_mutex(tp->is_paused);
	int i = 0;
	for(;i<thread_count;i++) destroy_worker(tp->worker_threads[i]);
	free(tp->worker_threads);
	free(tp);
	tp = NULL;
	MU_LOG_VERBOSE(logger, "Thread pool has been properly destroyed!\n");
	MU_Logger_Destroy(logger);
	return 1;
}

int Thread_Pool_Pause(void){
	if(!tp || tp->paused) return 0;
	tp->paused = 1;
	int successful_pauses = 0;
	int i = 0;
	for(;i<tp->thread_count;i++) if (pthread_kill(*tp->worker_threads[i]->thread, SIGUSR1) == 0) successful_pauses++;
	// The only time this function is successful is if it successfully paused all threads.
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
	return pthread_cond_broadcast(tp->resume) == 0;	
}

/* Undefine all user macros below. */

#undef TP_INCREMENT
#undef TP_DECREMENT