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
#include "scoped_lock.h"

struct c_utils_thread_pool {
	/// Array of threads.
	pthread_t *workers;
	/// The queue with all jobs assigned to it.
	struct c_utils_blocking_queue *queue;
	/// Amount of threads currently created, A.K.A Max amount.
	_Atomic size_t thread_count;
	/// Amount of threads currently active.
	_Atomic size_t active_threads;
	/// Flags used to keep track of internal state.
	volatile int flags;
	/// Used for timed pauses.
	struct timespec pause_time;
	/// Used to synchronize changes to pause_time and flags
	struct c_utils_scoped_lock *plock;
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
	/// Pre-Computed time was added in seconds.
	long added_sec;
	/// Pre-Computed time was added in milliseconds.
	long added_msec;
	/// c_utils_priority of task.
	int priority;
};

static const char *pause_event_name = "Resume";
static const char *finished_event_name = "Finished";
static const char *result_event_name = "Result Ready";

static const int KEEP_ALIVE = 1 << 0;
static const int INIT_ERR = 1 << 1;
static const int SHUTDOWN = 1 << 2;
static const int PAUSED = 1 << 3;

/* Begin Static, Private functions */

static void configure(struct c_utils_thread_pool_conf *conf);

static void process_task(struct c_utils_thread_task *task);

static void *get_tasks(void *args);

static int compare_task_priority(const void *task_one, const void *task_two);

static void destroy_thread_pool(void *instance);

static bool is_infinite_timeout(struct timespec *timeout);

static void make_timeout_infinite(struct timespec *timeout);

/* End Static, Private functions. */

struct c_utils_thread_pool *c_utils_thread_pool_create() {
	struct c_utils_thread_pool_conf conf = {0};
	return c_utils_thread_pool_create_conf(&conf);
}

struct c_utils_thread_pool *c_utils_thread_pool_create_conf(struct c_utils_thread_pool_conf *conf) {
	if(!conf)
		return NULL;

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

	tp->finished = c_utils_event_create(finished_event_name, conf->logger);
	if (!tp->finished) {
		C_UTILS_LOG_ERROR(conf->logger, "c_utils_event_create: 'Was unable to create event: %s!'", finished_event_name);
		goto err_finished;
	}

	tp->plock = c_utils_scoped_lock_spinlock(0, conf->logger);
	if(!tp->plock) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed in creation of the scoped_lock!");
		goto err_plock;
	}

	C_UTILS_ON_BAD_MALLOC(tp->workers, conf->logger, sizeof(*tp->workers) * conf->num_threads)
		goto err_workers;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	for (size_t i = 0; i < conf->num_threads; i++) {
		int create_error = pthread_create(tp->workers + i, &attr, get_tasks, tp);
		if (create_error) {
			C_UTILS_LOG_ERROR(conf->logger, "pthread_create: '%s'", strerror(create_error));
			goto err_worker_alloc;
		}
		
		tp->thread_count++;
	}

	pthread_attr_destroy(&attr);
	tp->flags = KEEP_ALIVE;

	return tp;

	err_worker_alloc:
		// Since the threads are polling until keep_alive is true, this informs them that they should exit.
		tp->flags |= INIT_ERR;

		free(tp->workers);

		// Wait for all threads to exit
		while (atomic_load(&tp->thread_count))
					pthread_yield();
	err_workers:
		c_utils_scoped_lock_destroy(tp->plock);
	err_plock:
		c_utils_event_destroy(tp->finished);
	err_finished:
		c_utils_event_destroy(tp->resume);
	err_resume:
		c_utils_blocking_queue_destroy(tp->queue);
	err_queue:
		free(tp);
	err:
		return NULL;
}

bool c_utils_thread_pool_add(struct c_utils_thread_pool *tp, void *(*task)(void *), void *args, int priority) {
	if(!tp)
		return false;

	if(!task) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "Thread Pool cannot process a NULL task.");
		return false;
	}

	if(priority < -1) {
		C_UTILS_LOG_ERROR(tp->conf.logger, "Bad Thread Pool priority, requires range of -1 to 1000, received %d", priority);
		return false;
	}

	struct c_utils_thread_task *thread_task = NULL;

	C_UTILS_ON_BAD_CALLOC(thread_task, tp->conf.logger, sizeof(*thread_task))
		goto err;

	thread_task->callback = task;
	thread_task->args = args;
	thread_task->priority = priority;

	// Do not accept new tasks if shutdown.
	C_UTILS_SCOPED_LOCK(tp->plock) {
		if(tp->flags & SHUTDOWN)
			goto err_shutdown;

		c_utils_event_reset(tp->finished);
		bool enqueued = c_utils_blocking_queue_enqueue(tp->queue, thread_task, C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT);
		if (!enqueued) {
			C_UTILS_LOG_ERROR(tp->conf.logger, "blocking_queue_nqueue: 'Was unable to enqueue a thread_task!'");
			goto err_enqueue;
		}
	}

	C_UTILS_LOG_VERBOSE(tp->conf.logger, "A task of priority %d has been added to the task_queue!", priority);
	return true;

	err_shutdown:
	err_enqueue:
		free(task);
	err:
		return false;
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
		goto err_result_ready;
	}

	C_UTILS_ON_BAD_CALLOC(thread_task, tp->conf.logger, sizeof(*thread_task))
		goto err_task;
	
	thread_task->callback = task;
	thread_task->args = args;
	thread_task->priority = priority;
	thread_task->result = result;

	C_UTILS_SCOPED_LOCK(tp->plock) {
		if(tp->flags & SHUTDOWN)
			goto err_shutdown;

		c_utils_event_reset(tp->finished);
		bool enqueued = c_utils_blocking_queue_enqueue(tp->queue, thread_task, C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT);
		if (!enqueued) {
			C_UTILS_LOG_ERROR(tp->conf.logger, "blocking_queue_nqueue: 'Was unable to enqueue a thread_task!'");
			goto err_enqueue;
		}
	}

	C_UTILS_LOG_VERBOSE(tp->conf.logger, "A task of %d priority has been added to the task_queue!", priority);
	return result;

	err_shutdown:
	err_enqueue:
		free(task);
	err_task:
		c_utils_event_destroy(result->is_ready);
	err_result_ready:
		free(result);
	err_result:
		return NULL;
}

bool c_utils_thread_pool_clear(struct c_utils_thread_pool *tp) {
	C_UTILS_ARG_CHECK(tp->conf.logger, false, tp);

	C_UTILS_LOG_VERBOSE(tp->conf.logger, "Clearing all tasks!");
	
	c_utils_blocking_queue_delete_all(tp->queue);

	return true;
}

void c_utils_result_destroy(struct c_utils_result *result) {
	if(!result)
		return;

	c_utils_event_destroy(result->is_ready);
	free(result);
}

void *c_utils_result_get(struct c_utils_result *result, long long int timeout) {
	if(!result)
		return NULL;
	
	return c_utils_event_wait_for(result->is_ready, timeout) ? result->retval : NULL;
}

bool c_utils_thread_pool_wait_for(struct c_utils_thread_pool *tp, long long int timeout) {
	if(!tp)
		return false;

	return c_utils_event_wait_for(tp->finished, timeout);
}

void c_utils_thread_pool_wait_until(struct c_utils_thread_pool *tp, struct timespec *timeout) {
	if(!tp)
		return;

	c_utils_event_wait_until(tp->finished, timeout);
}

void c_utils_thread_pool_destroy(struct c_utils_thread_pool *tp) {
	if(!tp)
		return;

	if(tp->conf.flags & C_UTILS_THREAD_POOL_RC_INSTANCE) {
		C_UTILS_REF_DEC(tp);
		return;
	}

	destroy_thread_pool(tp);
}

void c_utils_thread_pool_pause_for(struct c_utils_thread_pool *tp, long long int timeout) {
	if(!tp)
		return;

	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);

	/*
		As we are dealing with a physical struct here, we cannot reliably set it to NULL, so we
		just either fill out the timespec if it's greater than 0, or zero the struct to allow for
		a more 
	*/
	if(timeout > 0) {
		tv.tv_sec += timeout / 1000;
		tv.tv_nsec += (timeout % 1000) * 1000000L;
	}

	c_utils_thread_pool_pause_until(tp, timeout != C_UTILS_THREAD_POOL_NO_TIMEOUT ? &tv : NULL);
}

void c_utils_thread_pool_pause_until(struct c_utils_thread_pool *tp, struct timespec *timeout) {
	if(!tp)
		return;

	C_UTILS_SCOPED_LOCK(tp->plock) {
		tp->flags |= PAUSED;
		if(!timeout)
			make_timeout_infinite(&tp->pause_time);
		else
			tp->pause_time = *timeout;
	}
}

void c_utils_thread_pool_resume(struct c_utils_thread_pool *tp) {
	if(!tp)
		return;

	C_UTILS_SCOPED_LOCK(tp->plock)
		tp->flags &= ~PAUSED;

	return c_utils_event_signal(tp->resume);
}

void c_utils_thread_pool_shutdown(struct c_utils_thread_pool *tp) {
	if(!tp)
		return;

	C_UTILS_SCOPED_LOCK(tp->plock) {
		tp->flags |= SHUTDOWN;

	}
}



static void destroy_thread_pool(void *instance) {
	struct c_utils_thread_pool *tp = instance;

	tp->flags &= ~KEEP_ALIVE;

	// By destroying the PBQueue, it signals to threads waiting to wake up.
	c_utils_blocking_queue_destroy(tp->queue);
	// Then by destroying the resume event, anything waiting on a paused thread pool wakes up.
	c_utils_event_destroy(tp->resume);
	// Then we wait for all threads to that wake up to exit gracefully.
	c_utils_thread_pool_wait_for(tp, C_UTILS_THREAD_POOL_NO_TIMEOUT);
	// Finally, any threads waiting on the thread pool to finish will wake up.
	c_utils_event_destroy(tp->finished);

	while (atomic_load(&tp->active_threads))   
		pthread_yield();
	
	free(tp->workers);
	free(tp);
}

static void make_timeout_infinite(struct timespec *timeout) {
	timeout->tv_sec = 0;
	timeout->tv_nsec = 0;
}

static bool is_infinite_timeout(struct timespec *timeout) {
	return timeout->tv_sec == 0 && timeout->tv_nsec == 0;
}

static int compare_task_priority(const void *task_one, const void *task_two) {
	const struct c_utils_thread_task *first = task_one, *second = task_two;
	const int FIRST_GREATER = 1, SECOND_GREATER = -1;

	/*
		The thread listed with the priority of IMMEDIATE will jump in front of all other tasks... UNLESS the other thread
		also is of THREAD_POOL_IMMEDIATE priority, upon which the one added first will get to go first.
	*/
	if(first->priority == C_UTILS_THREAD_POOL_PRIORITY_IMMEDIATE && second->priority != C_UTILS_THREAD_POOL_PRIORITY_IMMEDIATE)
		return FIRST_GREATER;

	if(second->priority == C_UTILS_THREAD_POOL_PRIORITY_IMMEDIATE && first->priority != C_UTILS_THREAD_POOL_PRIORITY_IMMEDIATE)
		return SECOND_GREATER;

	/*
		If both are are IMMEDIATE or neither are IMMEDIATE, we go based on time added and priority. Time for each is aged according to
		both the priority. Note that since two IMMEDIATE priority tasks have the same priority value, it doesn't matter much in the
		below calculations.
	*/

	long first_aged_sec = first->added_sec - (first->priority / 1000), second_aged_sec = second->added_sec - (second->priority / 1000);
	long first_aged_msec = first->added_msec - (first->priority % 1000) * 1000000L, second_aged_msec = second->added_msec - (second->priority % 1000) * 1000000L;

	if((first_aged_sec < second_aged_sec) || (first_aged_sec == second_aged_sec && first_aged_msec < second_aged_msec))
		return FIRST_GREATER;
	else
		return SECOND_GREATER;
}

static void *get_tasks(void *args) {
	struct c_utils_thread_pool *tp = args;

	// keep_alive thread is initialized to 0 meaning it isn't setup, but set to 1 after it is, so it is dual-purpose.
	while (!(tp->flags & KEEP_ALIVE) && !(tp->flags & INIT_ERR))  
		pthread_yield();
	
	if (tp->flags & INIT_ERR) {
		// If there is an initialization error, we need to abort ASAP as the thread actually gets freed, so we can't risk dereferencing self.
		atomic_fetch_sub(&tp->thread_count, 1);
		pthread_exit(NULL);
	}

	// Note that this while loop checks for keep_alive to be true, rather than false. 
	while (tp->flags & KEEP_ALIVE) {
		struct c_utils_thread_task *task = NULL;
		task = c_utils_blocking_queue_dequeue(tp->queue, C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT);

		if (!(tp->flags & KEEP_ALIVE))
			break;
		
		struct timespec timeout;
		bool is_paused;
		C_UTILS_SCOPED_LOCK(tp->plock) {
			is_paused = tp->flags & PAUSED;
			timeout = tp->pause_time;
		}

		if(is_paused)
			c_utils_event_wait_until(tp->resume, is_infinite_timeout(&timeout) ? NULL : &timeout);

		// Also note that if we do wait for a period of time, the thread pool could be set to shut down.
		if (!(tp->flags & KEEP_ALIVE)) {
			free(task);
			break;
		}

		atomic_fetch_add(&tp->active_threads, 1);
		process_task(task);
		atomic_fetch_sub(&tp->active_threads, 1);

		// Close as we are going to get testing if the queue is fully empty. 
		if (c_utils_blocking_queue_size(tp->queue) == 0 && atomic_load(&tp->active_threads) == 0)  
			c_utils_event_signal(tp->finished);
	}

	atomic_fetch_sub(&tp->thread_count, 1);
	C_UTILS_LOG_VERBOSE(tp->conf.logger, "A thread exited!\n");

	return NULL;
}

static void configure(struct c_utils_thread_pool_conf *conf) {
	if(!conf->num_threads) {
		conf->num_threads = sysconf(_SC_NPROCESSORS_ONLN);
	}
}

static void process_task(struct c_utils_thread_task *task) {
	if (!task) 
		return;

	void *retval = task->callback(task->args);
	if (task->result) {
		task->result->retval = retval;
		c_utils_event_signal(task->result->is_ready);
	}

	free(task);
}