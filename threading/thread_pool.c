#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdatomic.h>
#include <unistd.h>

#include "events.h"
#include "../misc/argument_check.h"
#include "../misc/alloc_check.h"
#include "../misc/flags.h"
#include "../io/logger.h"
#include "thread_pool.h"
#include "../data_structures/blocking_queue.h"
#include "../memory/ref_count.h"

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
	struct c_utils_blocking_queue *queue;
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
	/// Configuration Object
	struct c_utils_thread_pool_conf conf;
};

struct c_utils_result {
	/// Determines if c_utils_result is ready.
	struct c_utils_event *is_ready;
	/// The returned item from the task.
	void *retval;
};

struct c_utils_thread_task {
	/// Task to be executed.
	void *(*callback)(void *);
	/// Arguments to be passed to the task.
	void *args;
	/// c_utils_result from the Task.
	struct c_utils_result *result;
	/// c_utils_priority of task.
	int priority;
};

static const char *pause_event_name = "Resume";
static const char *finished_event_name = "Finished";
static const char *result_event_name = "Result Ready";

/* Begin Static, Private functions */

static void configure(struct c_utils_thread_pool_conf *conf) {
	if(!conf->num_threads) {
		conf->num_threads = sysconf(_SC_NPROCESSORS_ONLN);
	}
}

/// Destructor for a task.
static void destroy_task(struct c_utils_thread_task *task) {
	if (!task)
		return;
	
	if (task->result) {
		c_utils_event_destroy(task->result->is_ready, 0);
		free(task->result);
	}

	free(task);
}

static void destroy_worker(struct c_utils_thread_worker *worker) {
	if (!worker) 
		return;

	free(worker->thread);
	free(worker);
}

/// Return the thread_worker associated with the calling thread.
static struct c_utils_thread_worker *get_self(struct c_utils_thread_pool *tp) {
	pthread_t self = pthread_self();
	int thread_count = tp->thread_count;

	for (int i = 0;i<thread_count;i++)
		if (pthread_equal(*(tp->worker_threads[i]->thread), self))  
			return tp->worker_threads[i];

	C_UTILS_LOG_ERROR(tp->conf.logger, "A worker thread could not be identified from pthread_self!");
	
	return NULL;
}


static void process_task(struct c_utils_thread_task *task, unsigned int thread_id) {
	if (!task) 
		return;

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
		struct c_utils_thread_task *task = NULL;
		task = c_utils_blocking_queue_dequeue(tp->queue, C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT);

		if (!tp->keep_alive)  
			break;
		

		// Note that the paused event is only waited upon if it the event interally is flagged.
		c_utils_event_wait(tp->resume, tp->seconds_to_pause, self->thread_id);

		// Also note that if we do wait for a period of time, the thread pool could be set to shut down.
		if (!tp->keep_alive)
			break;
		

		atomic_fetch_add(&tp->active_threads, 1);
		C_UTILS_LOG_VERBOSE(tp->conf.logger, "Thread #%d: received a task!", self->thread_id);
		process_task(task, self->thread_id);
		C_UTILS_LOG_VERBOSE(tp->conf.logger, "Thread #%d: finished a task!", self->thread_id);
		atomic_fetch_sub(&tp->active_threads, 1);

		// Close as we are going to get testing if the queue is fully empty. 
		if (c_utils_blocking_queue_size(tp->queue) == 0 && atomic_load(&tp->active_threads) == 0)  
			c_utils_event_signal(tp->finished, self->thread_id);
	}
	atomic_fetch_sub(&tp->thread_count, 1);
	C_UTILS_LOG_VERBOSE(tp->conf.logger, "Thread #%d: Exited!\n", self->thread_id);

	return NULL;
}

/// Simple comparator to compare two task priorities.
static int compare_task_priority(const void *task_one, const void *task_two) {
	return ((struct c_utils_thread_task *)task_one)->priority - ((struct c_utils_thread_task *)task_two)->priority;
}

static void destroy_thread_pool(void *instance);

/* End Static, Private functions. */

struct c_utils_thread_pool *c_utils_thread_pool_create() {
	struct c_utils_thread_pool_conf conf = {0};
	return c_utils_thread_pool_create_conf(&conf);
}

struct c_utils_thread_pool *c_utils_thread_pool_create_conf(struct c_utils_thread_pool_conf *conf) {
	if(!conf)
		return NULL;

	size_t workers_allocated = 0;

	configure(conf);

	struct c_utils_thread_pool *tp;
	if(conf->flags & C_UTILS_THREAD_POOL_RC_INSTANCE) {
		struct c_utils_ref_count_conf rc_conf =
		{
			.logger = conf->logger,
			.destructor = destroy_thread_pool
		};

		tp = c_utils_ref_create_conf(sizeof(*tp), &rc_conf);
	} else {
		tp = malloc(sizeof(*tp));
	}

	if(!tp)
		goto err;

	tp->thread_count = ATOMIC_VAR_INIT(0);
	tp->active_threads = ATOMIC_VAR_INIT(0);

	struct c_utils_blocking_queue_conf bq_conf =
	{
		.logger = conf->logger,
		.callbacks.comparators.item = compare_task_priority,
		.flags = C_UTILS_BLOCKING_QUEUE_DELETE_ON_DESTROY
	};

	tp->queue = c_utils_blocking_queue_create_conf(&bq_conf);
	if (!tp->queue) {
		C_UTILS_LOG_ERROR(conf->logger, "c_utils_blocking_queue_create: 'Was unable to create blocking queue!'");
		goto err_queue;
	}

	tp->resume = c_utils_event_create(pause_event_name, conf->logger, C_UTILS_EVENT_SIGNALED_BY_DEFAULT | C_UTILS_EVENT_SIGNAL_ON_TIMEOUT);
	if (!tp->resume) {
		C_UTILS_LOG_ERROR(conf->logger, "c_utils_event_create: 'Was unable to create event: %s!'", pause_event_name);
		goto err_resume;
	}

	tp->finished = c_utils_event_create(finished_event_name, conf->logger, 0);
	if (!tp->finished) {
		C_UTILS_LOG_ERROR(conf->logger, "c_utils_event_create: 'Was unable to create event: %s!'", finished_event_name);
		goto err_finished;
	}

	C_UTILS_ON_BAD_MALLOC(tp->worker_threads, conf->logger, sizeof(*tp->worker_threads) * conf->num_threads)
		goto err_workers;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	for (size_t i = 0;i < conf->num_threads; i++) {
		struct c_utils_thread_worker *worker;
		C_UTILS_ON_BAD_CALLOC(worker, conf->logger, sizeof(*worker))
			goto err_worker_alloc;

		workers_allocated++;
		
		C_UTILS_ON_BAD_MALLOC(worker->thread, conf->logger, sizeof(*worker->thread))
			goto err_worker_alloc;

		worker->thread_id = i+1;
		
		int create_error = pthread_create(worker->thread, &attr, get_tasks, tp);
		if (create_error) {
			C_UTILS_LOG_ERROR(conf->logger, "pthread_create: '%s'", strerror(create_error));
			goto err_worker_alloc;
		}
		
		tp->worker_threads[i] = worker;
		tp->thread_count++;
	}

	pthread_attr_destroy(&attr);
	tp->keep_alive = true;

	return tp;

	err_worker_alloc:
		// Since the threads are polling until keep_alive is true, this informs them that they should exit.
		tp->init_error = true;

		for(size_t i = 0; i < workers_allocated; i++) {
			struct c_utils_thread_worker *worker = tp->worker_threads[i];
			free(worker->thread);
			free(worker);
		}

		// Wait for all threads to exit
		while (atomic_load(&tp->thread_count))
					pthread_yield();

		free(tp->worker_threads);
	err_workers:
		c_utils_event_destroy(tp->finished, 0);
	err_finished:
		c_utils_event_destroy(tp->resume, 0);
	err_resume:
		c_utils_blocking_queue_destroy(tp->queue);
	err_queue:
		free(tp);
	err:
		return NULL;
}

void c_utils_thread_pool_add(struct c_utils_thread_pool *tp, void *(*task)(void *), void *args, int priority) {
	if(!tp)
		return;

	if(!task) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "Thread Pool cannot process a NULL task.");
		return;
	}

	if(priority < -1) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "Bad Thread Pool priority, requires range of -1 to 1000, received %d", priority);
		return;
	}

	struct c_utils_result *result = NULL;
	struct c_utils_thread_task *thread_task = NULL;

	C_UTILS_ON_BAD_CALLOC(thread_task, tp->conf.logger, sizeof(*thread_task))
		goto err_task;

	thread_task->callback = task;
	thread_task->args = args;
	thread_task->priority = priority;

	c_utils_event_reset(tp->finished, 0);
	bool enqueued = c_utils_blocking_queue_enqueue(tp->queue, thread_task, C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT);
	if (!enqueued) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "PBQueue_Enqueue: 'Was unable to enqueue a thread_task!'");
		goto err_enqueue;
	}

	C_UTILS_LOG_VERBOSE(tp->conf.logger, "A task of priority %d has been added to the task_queue!", priority);
	return;

	err_enqueue:
		free(task);
	err_task:
		free(result);
}

struct c_utils_result *c_utils_thread_pool_add_for_result(struct c_utils_thread_pool *tp, void *(*task)(void *), void *args, int priority) {
	if(!tp)
		return NULL;

	if(!task) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "Thread Pool cannot process a NULL task.");
		return NULL;
	}

	if(priority < -1) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "Bad Thread Pool priority, requires range of -1 to 1000, received %d", priority);
		return NULL;
	}

	struct c_utils_result *result = NULL;
	struct c_utils_thread_task *thread_task = NULL;

	C_UTILS_ON_BAD_CALLOC(result, tp->conf.logger, sizeof(*result))
		goto err_result;

	result->is_ready = c_utils_event_create(result_event_name, tp->conf.logger, 0);
	if (!result->is_ready) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "c_utils_event_create: 'Was unable to create event: %s!'", result_event_name);
		goto err_result;
	}

	C_UTILS_ON_BAD_CALLOC(thread_task, tp->conf.logger, sizeof(*thread_task))
		goto err_task;
	
	thread_task->callback = task;
	thread_task->args = args;
	thread_task->priority = priority;
	thread_task->result = result;

	c_utils_event_reset(tp->finished, 0);
	bool enqueued = c_utils_blocking_queue_enqueue(tp->queue, thread_task, -1);
	if (!enqueued) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "PBQueue_Enqueue: 'Was unable to enqueue a thread_task!'");
		goto err_enqueue;
	}

	C_UTILS_LOG_VERBOSE(tp->conf.logger, "A task of %d priority has been added to the task_queue!", priority);
	return result;

	err_enqueue:
		free(task);
	err_task:
	err_result:
		if(result)
			if(result->is_ready)
				c_utils_event_destroy(result->is_ready, 0);

		free(result);

		return NULL;
}

bool c_utils_thread_pool_clear(struct c_utils_thread_pool *tp) {
	C_UTILS_ARG_CHECK(tp->conf.logger, false, tp);

	C_UTILS_LOG_VERBOSE(tp->conf.logger, "Clearing all tasks from Thread Pool!");
	
	c_utils_blocking_queue_delete_all(tp->queue);

	return true;
}

/// Will destroy the result and associated event.
void c_utils_result_destroy(struct c_utils_result *result) {
	if(!result)
		return;

	c_utils_event_destroy(result->is_ready, 0);
	free(result);
}
/// Will block until result is ready.
void *c_utils_result_get(struct c_utils_result *result, long long int timeout) {
	if(!result)
		return NULL;

	bool is_ready = c_utils_event_wait(result->is_ready, timeout, 0);
	
	return is_ready ? result->retval : NULL;
}

/// Will block until all tasks are finished.
bool c_utils_thread_pool_wait(struct c_utils_thread_pool *tp, long long int timeout) {
	C_UTILS_ARG_CHECK(tp->conf.logger, false, tp);

	bool is_finished = c_utils_event_wait(tp->finished, timeout, 0);
	
	return is_finished;
}

bool c_utils_thread_pool_destroy(struct c_utils_thread_pool *tp) {
	C_UTILS_ARG_CHECK(tp->conf.logger, false, tp);

	tp->keep_alive = false;
	size_t old_thread_count = tp->thread_count;

	// By destroying the PBQueue, it signals to threads waiting to wake up.
	c_utils_blocking_queue_destroy(tp->queue);
	// Then by destroying the resume event, anything waiting on a paused thread pool wakes up.
	c_utils_event_destroy(tp->resume, 0);
	// Then we wait for all threads to that wake up to exit gracefully.
	c_utils_thread_pool_wait(tp, -1);
	// Finally, any threads waiting on the thread pool to finish will wake up.
	c_utils_event_destroy(tp->finished, 0);

	while (atomic_load(&tp->active_threads))   
		pthread_yield();
	
	for (int i = 0; i < old_thread_count; i++)   
		destroy_worker(tp->worker_threads[i]);
	
	free(tp->worker_threads);
	free(tp);
	
	C_UTILS_LOG_VERBOSE(tp->conf.logger, "Thread pool has been properly destroyed!\n");
	return true;
}

bool c_utils_thread_pool_pause(struct c_utils_thread_pool *tp, long long int timeout) {
	C_UTILS_ARG_CHECK(tp->conf.logger, false, tp);
	
	tp->seconds_to_pause = timeout;
	
	return c_utils_event_reset(tp->resume, 0);
}

bool c_utils_thread_pool_resume(struct c_utils_thread_pool *tp) {
	C_UTILS_ARG_CHECK(tp->conf.logger, false, tp);

	return c_utils_event_signal(tp->resume, 0);
}