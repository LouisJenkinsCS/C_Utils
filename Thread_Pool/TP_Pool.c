#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <MU_Arg_Check.h>
#include <MU_Flags.h>
#include <MU_Logger.h>
#include <TP_Pool.h>

typedef enum {
	/// Lowest possible priority
	TP_LOWEST,
	/// Low priority, above Lowest.
	TP_LOW,
	/// Medium priority, considered the "average".
	TP_MEDIUM,
	/// High priority, or "above average".
	TP_HIGH,
	/// Highest priority, or "imminent"
	TP_HIGHEST
} TP_Priority_e;

typedef struct {
	/// Task to be executed.
	TP_Callback callback;
	/// Arguments to be passed to the task.
	void *args;
	/// Result from the Task.
	TP_Result_t *result;
	/// Priority of task.
	TP_Priority_e priority;
} TP_Task_t;

static const char *pause_event_name = "Resume";
static const char *finished_event_name = "Finished";
static const char *result_event_name = "Result Ready";

static MU_Logger_t *logger = NULL;
static MU_Logger_t *event_logger = NULL;

MU_LOGGER_AUTO_CREATE(logger, "./Thread_Pool/Logs/TP_Pool.log", "w", MU_ALL);

MU_LOGGER_AUTO_CREATE(event_logger, "./Thread_Pool/Logs/TP_Pool_Events.log", "w", MU_ALL);

/* Begin Static, Private functions */

/// Destructor for a task.
static void Destroy_Task(TP_Task_t *task){
	if(!task) return;
	if(task->result){
		MU_Event_destroy(task->result->is_ready, 0);
		free(task->result);
	}
	free(task);
}

static void Destroy_Worker(TP_Worker_t *worker){
	if(!worker) return;
	free(worker->thread);
	free(worker);
}

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
	MU_LOG_ERROR(logger, "A worker thread could not be identified from pthread_self!");
	return NULL;
}


static void Process_Task(TP_Task_t *task, unsigned int thread_id){
	if(!task) return;
	void *retval = task->callback(task->args);
	if(task->result){
		task->result->retval = retval;
		MU_Event_signal(task->result->is_ready, thread_id);
	}
	free(task);
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
		task = DS_PBQueue_dequeue(tp->queue, -1);
		if(!tp->keep_alive){
			break;
		}
		// Note that the paused event is only waited upon if it the event interally is flagged.
		MU_Event_wait(tp->resume, tp->seconds_to_pause, self->thread_id);
		// Also note that if we do wait for a period of time, the thread pool could be set to shut down.
		if(!tp->keep_alive){
			break;
		}
		atomic_fetch_add(&tp->active_threads, 1);
		MU_LOG_VERBOSE(logger, "Thread #%d: received a task!", self->thread_id);
		Process_Task(task, self->thread_id);
		MU_LOG_VERBOSE(logger, "Thread #%d: finished a task!", self->thread_id);
		atomic_fetch_sub(&tp->active_threads, 1);
		// Close as we are going to get testing if the queue is fully empty. 
		if(DS_PBQueue_size(tp->queue) == 0 && atomic_load(&tp->active_threads) == 0){
			MU_Event_signal(tp->finished, self->thread_id);
		}
	}
	atomic_fetch_sub(&tp->thread_count, 1);
	MU_LOG_VERBOSE(logger, "Thread #%d: Exited!\n", self->thread_id);
	return NULL;
}

/// Is used to obtain the priority from the flag and set the task's priority to it. Has to be done this way to allow for bitwise.
static void Set_Task_Priority(TP_Task_t *task, int flags){
	if(MU_FLAG_GET(flags, TP_LOWEST_PRIORITY)) task->priority = TP_LOWEST;
	else if(MU_FLAG_GET(flags, TP_LOW_PRIORITY)) task->priority = TP_LOW;
	else if(MU_FLAG_GET(flags, TP_HIGH_PRIORITY)) task->priority = TP_HIGH;
	else if(MU_FLAG_GET(flags, TP_HIGHEST_PRIORITY)) task->priority = TP_HIGHEST;
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
	tp->queue = DS_PBQueue_create(0, (void *)compare_task_priority);
	tp->resume = MU_Event_create(pause_event_name, event_logger, MU_EVENT_SIGNALED_BY_DEFAULT | MU_EVENT_SIGNAL_ON_TIMEOUT);
	if(!tp->resume){
		MU_LOG_ERROR(logger, "MU_Event_create: 'Was unable to create event: %s!'", pause_event_name);
		goto error;
	}
	tp->finished = MU_Event_create(finished_event_name, event_logger, 0);
	if(!tp->finished){
		MU_LOG_ERROR(logger, "MU_Event_create: 'Was unable to create event: %s!'", finished_event_name);
		goto error;
	}
	tp->worker_threads = malloc(sizeof(pthread_t *) * pool_size);
	if(!tp->worker_threads){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	size_t i = 0;
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
		int create_error = pthread_create(worker->thread, &attr, Get_Tasks, tp);
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
			MU_Event_destroy(tp->resume, 0);
			MU_Event_destroy(tp->finished, 0);
			if(tp->worker_threads){
				size_t i = 0;
				for(; i < workers_allocated; i++){
					TP_Worker_t *worker = tp->worker_threads[i];
					free(worker->thread);
					free(worker);
				}
				while(atomic_load(&tp->thread_count)){
					pthread_yield();
				}
				free(tp->worker_threads);
			}
			free(tp);
		}
		return NULL;
}

TP_Result_t *TP_Pool_add(TP_Pool_t *tp, TP_Callback callback, void *args, int flags){
	MU_ARG_CHECK(logger, NULL, tp, callback);
	TP_Result_t *result = NULL;
	TP_Task_t *task = NULL;
	if(!MU_FLAG_GET(flags, TP_NO_RESULT)){
		result = calloc(1, sizeof(TP_Result_t));
		if(!result){
			MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
		result->is_ready = MU_Event_create(result_event_name, event_logger, 0);
		if(!result->is_ready){
			MU_LOG_ERROR(logger, "MU_Event_create: 'Was unable to create event: %s!'", result_event_name);
			goto error;
		}
	}
	task = calloc(1, sizeof(TP_Task_t));
	if(!task){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	task->callback = callback;
	task->args = args;
	Set_Task_Priority(task, flags);
	task->result = result;
	MU_Event_reset(tp->finished, 0);
	bool enqueued = DS_PBQueue_enqueue(tp->queue, task, -1);
	if(!enqueued){
		MU_LOG_ERROR(logger, "PBQueue_Enqueue: 'Was unable to enqueue task!'");
		goto error;
	}
	MU_LOG_VERBOSE(logger, "A task of %s priority has been added to the task_queue!", Get_Task_Priority(task));
	return result;

	error:
		if(result){
			if(result->is_ready){
				MU_Event_destroy(result->is_ready, 0);
			}
			free(result);
		}
		if(task){
			free(task);
		}
		return NULL;
}

bool TP_Pool_clear(TP_Pool_t *tp){
	MU_ARG_CHECK(logger, false, tp);
	MU_LOG_VERBOSE(logger, "Clearing all tasks from Thread Pool!");
	return DS_PBQueue_clear(tp->queue, (void *)Destroy_Task);
}

/// Will destroy the Result and set it's reference to NULL.
bool TP_Result_destroy(TP_Result_t *result){
	MU_ARG_CHECK(logger, NULL, result);
	MU_Event_destroy(result->is_ready, 0);
	free(result);
	return true;
}
/// Will block until result is ready. 
void *TP_Result_get(TP_Result_t *result, long long int timeout){
	MU_ARG_CHECK(logger, NULL, result);
	bool is_ready = MU_Event_wait(result->is_ready, timeout, 0);
	return is_ready ? result->retval : NULL;
}

/// Will block until all tasks are finished.
bool TP_Pool_wait(TP_Pool_t *tp, long long int timeout){
	MU_ARG_CHECK(logger, false, tp);
	bool is_finished = MU_Event_wait(tp->finished, timeout, 0);
	return is_finished;
}

bool TP_Pool_destroy(TP_Pool_t *tp){
	MU_ARG_CHECK(logger, false, tp);
	tp->keep_alive = false;
	size_t old_thread_count = tp->thread_count;
	// By destroying the PBQueue, it signals to threads waiting to wake up.
	DS_PBQueue_destroy(tp->queue, (void *)Destroy_Task);
	// Then by destroying the resume event, anything waiting on a paused thread pool wakes up.
	MU_Event_destroy(tp->resume, 0);
	// Then we wait for all threads to that wake up to exit gracefully.
	TP_Pool_wait(tp, -1);
	// Finally, any threads waiting on the thread pool to finish will wake up.
	MU_Event_destroy(tp->finished, 0);
	int i = 0;
	while(atomic_load(&tp->active_threads)) pthread_yield();
	for(; i < old_thread_count; i++) Destroy_Worker(tp->worker_threads[i]);
	free(tp->worker_threads);
	free(tp);
	MU_LOG_VERBOSE(logger, "Thread pool has been properly destroyed!\n");
	return true;
}

bool TP_Pool_pause(TP_Pool_t *tp, long long int timeout){
	MU_ARG_CHECK(logger, false, tp);
	tp->seconds_to_pause = timeout;
	bool event_reset = MU_Event_reset(tp->resume, 0);
	return event_reset;
}

bool TP_Pool_resume(TP_Pool_t *tp){
	MU_ARG_CHECK(logger, false, tp);
	return MU_Event_signal(tp->resume, 0);
}

/* Undefine all user macros below. */

#undef TP_INCREMENT
#undef TP_DECREMENT