#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdatomic.h>

#include "events.h"
#include "../misc/argument_check.h"
#include "../misc/flags.h"
#include "../io/logger.h"
#include "thread_pool.h"
#include "../data_structures/priority_queue.h"


enum c_utils_priority {
	/// Lowest possible c_utils_priority
	PRIORITY_LOWEST,
	/// Low c_utils_priority, above Lowest.
	PRIORITY_LOW,
	/// Medium c_utils_priority, considered the "average".
	PRIORITY_MEDIUM,
	/// High c_utils_priority, or "above average".
	PRIORITY_HIGH,
	/// Highest c_utils_priority, or "imminent"
	PRIORITY_HIGHEST
};

struct c_utils_thread_worker {
	/// The worker thread that does the work.
	pthread_t *thread;
	/// The worker thread id.
	unsigned int thread_id;
};

struct c_utils_thread_pool {
	/// Array of threads.
	struct c_utils_thread_worker **worker_threads;
	/// The queue with all jobs assigned to it.
	struct c_utils_priority_queue *queue;
	/// Amount of threads currently created, A.K.A Max amount.
	_Atomic size_t thread_count;
	/// Amount of threads currently active.
	_Atomic size_t active_threads;
	/// Flag to keep all threads alive.
	volatile bool keep_alive;
	/// Used for timed pauses.
	volatile unsigned int seconds_to_pause;
	/// Used to signal an error occured during initialization
	volatile bool init_error;
	/// Event for pause/resume.
	struct c_utils_event *resume;
	/// Event for if the thread pool is finished at the moment.
	struct c_utils_event *finished;
};

struct c_utils_result {
	/// Determines if c_utils_result is ready.
	struct c_utils_event *is_ready;
	/// The returned item from the task.
	void *retval;
};

struct c_utils_thread_task {
	/// Task to be executed.
	c_utils_task callback;
	/// Arguments to be passed to the task.
	void *args;
	/// c_utils_result from the Task.
	struct c_utils_result *result;
	/// c_utils_priority of task.
	enum c_utils_priority priority;
};

static const char *pause_event_name = "Resume";
static const char *finished_event_name = "Finished";
static const char *result_event_name = "Result Ready";

static struct c_utils_logger *logger = NULL;
static struct c_utils_logger *event_logger = NULL;

LOGGER_AUTO_CREATE(logger, "./threading/logs/thread_pool.log", "w", C_UTILS_LOG_LEVEL_ALL);
LOGGER_AUTO_CREATE(event_logger, "./threading/logs/thread_pool_events.log", "w", C_UTILS_LOG_LEVEL_ALL);

/* Begin Static, Private functions */

/// Destructor for a task.
static void destroy_task(struct c_utils_thread_task *task) {
	if (!task) return;
	
	if (task->result) {
		c_utils_event_destroy(task->result->is_ready, 0);
		free(task->result);
	}
	free(task);
}

static void destroy_worker(struct c_utils_thread_worker *worker) {
	if (!worker) return;

	free(worker->thread);
	free(worker);
}

/// Return the Worker associated with the calling thread.
static struct c_utils_thread_worker *get_self(struct c_utils_thread_pool *tp) {
	pthread_t self = pthread_self();
	int thread_count = tp->thread_count;

	int i = 0;
	for (;i<thread_count;i++) {
		if (pthread_equal(*(tp->worker_threads[i]->thread), self))  
			return tp->worker_threads[i];
		
	}

	C_UTILS_LOG_ERROR(logger, "A worker thread could not be identified from pthread_self!");
	return NULL;
}


static void process_task(struct c_utils_thread_task *task, unsigned int thread_id) {
	if (!task) return;

	void *retval = task->callback(task->args);
	if (task->result) {
		task->result->retval = retval;
		c_utils_event_signal(task->result->is_ready, thread_id);
	}

	free(task);
}

/// The main thread loop to obtain tasks from the task queue.
static void *get_tasks(void *args) {
	struct c_utils_thread_pool *tp = args;

	// keep_alive thread is initialized to 0 meaning it isn't setup, but set to 1 after it is, so it is dual-purpose.
	while (!tp->init_error && !tp->keep_alive)  
		pthread_yield();
	
	if (tp->init_error) {
		// If there is an initialization error, we need to abort ASAP as the thread actually gets freed, so we can't risk dereferencing self.
		atomic_fetch_sub(&tp->thread_count, 1);
		pthread_exit(NULL);
	}

	struct c_utils_thread_worker *self = get_self(tp);
	// Note that this while loop checks for keep_alive to be true, rather than false. 
	while (tp->keep_alive) {
		c_utils_thread_task *task = NULL;
		task = c_utils_priority_queue_dequeue(tp->queue, -1);

		if (!tp->keep_alive)  
			break;
		

		// Note that the paused event is only waited upon if it the event interally is flagged.
		c_utils_event_wait(tp->resume, tp->seconds_to_pause, self->thread_id);

		// Also note that if we do wait for a period of time, the thread pool could be set to shut down.
		if (!tp->keep_alive)  
			break;
		

		atomic_fetch_add(&tp->active_threads, 1);
		C_UTILS_LOG_VERBOSE(logger, "Thread #%d: received a task!", self->thread_id);
		process_task(task, self->thread_id);
		C_UTILS_LOG_VERBOSE(logger, "Thread #%d: finished a task!", self->thread_id);
		atomic_fetch_sub(&tp->active_threads, 1);

		// Close as we are going to get testing if the queue is fully empty. 
		if (c_utils_priority_queue_size(tp->queue) == 0 && atomic_load(&tp->active_threads) == 0)  
			c_utils_event_signal(tp->finished, self->thread_id);
		
	}
	atomic_fetch_sub(&tp->thread_count, 1);
	C_UTILS_LOG_VERBOSE(logger, "Thread #%d: Exited!\n", self->thread_id);

	return NULL;
}

/// Is used to obtain the priority from the flag and set the task's priority to it. Has to be done this way to allow for bitwise.
static void set_priority(c_utils_thread_task *task, int flags) {
	if (FLAG_GET(flags, LOWEST_PRIORITY)) task->priority = PRIORITY_LOWEST;
	else if (FLAG_GET(flags, LOW_PRIORITY)) task->priority = PRIORITY_LOW;
	else if (FLAG_GET(flags, HIGH_PRIORITY)) task->priority = PRIORITY_HIGH;
	else if (FLAG_GET(flags, HIGHEST_PRIORITY)) task->priority = PRIORITY_HIGHEST;
	else task->priority = PRIORITY_MEDIUM;

}

static char *get_priority(c_utils_thread_task *task) {
	if (task->priority == PRIORITY_LOWEST) return "Lowest";
	else if (task->priority == PRIORITY_LOW) return "Low";
	else if (task->priority == PRIORITY_MEDIUM) return "Medium";
	else if (task->priority == PRIORITY_HIGH) return "High";
	else if (task->priority == PRIORITY_HIGHEST) return "Highest";
	else {
		C_UTILS_LOG_ERROR(logger, "Was unable to retrieve the priority of the task!\n");
		return NULL;
	}
}

/// Simple comparator to compare two task priorities.
static int compare_task_priority(void *task_one, void *task_two) {
	return ((struct c_utils_thread_task *)task_one)->priority - ((struct c_utils_thread_task *)task_two)->priority;
}

/* End Static, Private functions. */

struct c_utils_thread_pool *c_utils_thread_pool_create(size_t pool_size) {
	size_t workers_allocated = 0;

	struct c_utils_thread_pool *tp = calloc(1, sizeof(struct c_utils_thread_pool));
	if (!tp) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}

	tp->thread_count = ATOMIC_VAR_INIT(0);
	tp->active_threads = ATOMIC_VAR_INIT(0);

	tp->queue = c_utils_priority_queue_create(0, (void *)compare_task_priority);
	if (!tp->queue) {
		C_UTILS_LOG_ERROR(logger, "c_utils_priority_queue_create: 'Was unable to create prioritized event queue!'");
		goto error;
	}

	tp->resume = c_utils_event_create(pause_event_name, event_logger, MU_EVENT_SIGNALED_BY_DEFAULT | MU_EVENT_SIGNAL_ON_TIMEOUT);
	if (!tp->resume) {
		C_UTILS_LOG_ERROR(logger, "c_utils_event_create: 'Was unable to create event: %s!'", pause_event_name);
		goto error;
	}

	tp->finished = c_utils_event_create(finished_event_name, event_logger, 0);
	if (!tp->finished) {
		C_UTILS_LOG_ERROR(logger, "c_utils_event_create: 'Was unable to create event: %s!'", finished_event_name);
		goto error;
	}

	tp->worker_threads = malloc(sizeof(pthread_t *) * pool_size);
	if (!tp->worker_threads) {
		C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	size_t i = 0;
	for (;i < pool_size; i++) {
		struct c_utils_thread_worker *worker = calloc(1, sizeof(struct c_utils_thread_worker));
		if (!worker) {
			C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
		workers_allocated++;
		
		worker->thread = malloc(sizeof(pthread_t));
		if (!worker->thread) {
			C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}
		worker->thread_id = i+1;
		
		int create_error = pthread_create(worker->thread, &attr, get_tasks, tp);
		if (create_error) {
			C_UTILS_LOG_ERROR(logger, "pthread_create: '%s'", strerror(create_error));
			goto error;
		}
		
		tp->worker_threads[i] = worker;
		tp->thread_count++;
	}
	pthread_attr_destroy(&attr);
	tp->keep_alive = 1;
	
	return tp;

	error:
		if (tp) {
			tp->init_error = true;
			if (tp->queue)  
				c_utils_priority_queue_destroy(tp->queue);
			
			c_utils_event_destroy(tp->resume, 0);
			c_utils_event_destroy(tp->finished, 0);
			if (tp->worker_threads) {
				size_t i = 0;
				for (; i < workers_allocated; i++) {
					struct c_utils_thread_worker *worker = tp->worker_threads[i];
					free(worker->thread);
					free(worker);
				}
				while (atomic_load(&tp->thread_count))  
					pthread_yield();
				
				free(tp->worker_threads);
			}
			free(tp);
		}
		return NULL;
}

struct c_utils_result *c_utils_thread_pool_add(struct c_utils_thread_pool *tp, c_utils_task task, void *args, int flags) {
	C_UTILS_ARG_CHECK(logger, NULL, tp, callback);

	struct c_utils_result *result = NULL;
	struct c_utils_thread_task *thread_task = NULL;
	
	if (!FLAG_GET(flags, TU_NO_RESULT)) {
		result = calloc(1, sizeof(TU_Result_t));
		if (!result) {
			C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}

		result->is_ready = c_utils_event_create(result_event_name, event_logger, 0);
		if (!result->is_ready) {
			C_UTILS_LOG_ERROR(logger, "c_utils_event_create: 'Was unable to create event: %s!'", result_event_name);
			goto error;
		}

	}
	thread_task = calloc(1, sizeof(struct c_utils_thread_task));
	if (!task) {
		C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	
	thread_task->task = task;
	task->args = args;
	set_priority(thread_task, flags);
	thread_task->result = result;

	c_utils_event_reset(tp->finished, 0);
	bool enqueued = c_utils_priority_queue_enqueue(tp->queue, thread_task, -1);
	if (!enqueued) {
		C_UTILS_LOG_ERROR(logger, "PBQueue_Enqueue: 'Was unable to enqueue a thread_task!'");
		goto error;
	}

	C_UTILS_LOG_VERBOSE(logger, "A task of %s priority has been added to the task_queue!", get_priority(thread_task));
	return result;

	error:
		if (result) {
			if (result->is_ready)  
				c_utils_event_destroy(result->is_ready, 0);
			
			free(result);
		}
		if (thread_task)  
			free(thread_task);
		
		return NULL;
}

bool c_utils_thread_pool_clear(struct c_utils_thread_pool *tp) {
	C_UTILS_ARG_CHECK(logger, false, tp);

	C_UTILS_LOG_VERBOSE(logger, "Clearing all tasks from Thread Pool!");
	return c_utils_priority_queue_clear(tp->queue, (void *)destroy_task);
}

/// Will destroy the result and associated event.
bool c_utils_result_destroy(struct c_utils_result *result) {
	C_UTILS_ARG_CHECK(logger, NULL, result);

	c_utils_event_destroy(result->is_ready, 0);
	free(result);
	return true;
}
/// Will block until result is ready. 
void *c_utils_result_get(struct c_utils_result *result, long long int timeout) {
	C_UTILS_ARG_CHECK(logger, NULL, result);

	bool is_ready = c_utils_event_wait(result->is_ready, timeout, 0);
	return is_ready ? result->retval : NULL;
}

/// Will block until all tasks are finished.
bool c_utils_thread_pool_wait(struct c_utils_thread_pool *tp, long long int timeout) {
	C_UTILS_ARG_CHECK(logger, false, tp);

	bool is_finished = c_utils_event_wait(tp->finished, timeout, 0);
	return is_finished;
}

bool c_utils_thread_pool_destroy(struct c_utils_thread_pool *tp) {
	C_UTILS_ARG_CHECK(logger, false, tp);

	tp->keep_alive = false;
	size_t old_thread_count = tp->thread_count;

	// By destroying the PBQueue, it signals to threads waiting to wake up.
	c_utils_priority_queue_destroy(tp->queue, (void *)destroy_task);
	// Then by destroying the resume event, anything waiting on a paused thread pool wakes up.
	c_utils_event_destroy(tp->resume, 0);
	// Then we wait for all threads to that wake up to exit gracefully.
	c_utils_thread_pool_wait(tp, -1);
	// Finally, any threads waiting on the thread pool to finish will wake up.
	c_utils_event_destroy(tp->finished, 0);

	while (atomic_load(&tp->active_threads))   
		pthread_yield();
	

	int i = 0;
	for (; i < old_thread_count; i++)   
		destroy_worker(tp->worker_threads[i]);
	

	free(tp->worker_threads);
	free(tp);
	
	C_UTILS_LOG_VERBOSE(logger, "Thread pool has been properly destroyed!\n");
	return true;
}

bool c_utils_thread_pool_pause(struct c_utils_thread_pool *tp, long long int timeout) {
	C_UTILS_ARG_CHECK(logger, false, tp);
	
	tp->seconds_to_pause = timeout;
	return c_utils_event_reset(tp->resume, 0);
}

bool c_utils_thread_pool_resume(struct c_utils_thread_pool *tp) {
	C_UTILS_ARG_CHECK(logger, false, tp);

	return c_utils_event_signal(tp->resume, 0);
}