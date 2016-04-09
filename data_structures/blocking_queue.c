#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#include "../memory/ref_count.h"
#include "blocking_queue.h"
#include "../misc/argument_check.h"
#include "../misc/alloc_check.h"
#include "heap.h"
#include "list.h"

struct c_utils_blocking_queue {
	union {
		struct c_utils_list *list;
		struct c_utils_heap *heap;
	} data;
	/// A new element may be added to the PBQueue.
	pthread_cond_t removed;
	/// A element has been added and may be removed from the PBQueue.
	pthread_cond_t added;
	/// The lock used to add or remove an element to/from the queue (respectively).
	pthread_mutex_t lock;
	/// The amount of threads waiting.
	_Atomic size_t blocked;
	/// Atomic flag for if it's being destroyed.
	_Atomic bool shutdown;
	/// Configuration
	struct c_utils_blocking_queue_conf conf;
};

static bool add_item(struct c_utils_blocking_queue *bq, void *item) {
	if(bq->conf.callbacks.comparators.item)
		return c_utils_heap_insert(bq->data.heap, item);
	else
		return c_utils_list_add(bq->data.list, item);
}

static void *take_item(struct c_utils_blocking_queue *bq) {
	if(bq->conf.callbacks.comparators.item)
		return c_utils_heap_remove(bq->data.heap);
	else
		return c_utils_list_remove_at(bq->data.list, 0);
}

/* End static functions */

struct c_utils_blocking_queue *c_utils_blocking_queue_create() {
	struct c_utils_blocking_queue_conf conf = {};
	return c_utils_blocking_queue_create_conf(&conf);
}

struct c_utils_blocking_queue *c_utils_blocking_queue_create_conf(struct c_utils_blocking_queue_conf *conf) {
	if(!conf)
		return NULL;

	struct c_utils_blocking_queue *bq;

	if(conf->flags & C_UTILS_BLOCKING_QUEUE_RC_INSTANCE) {
		struct c_utils_ref_count_conf rc_conf =
		{
			.logger = conf->logger
		};

		bq = c_utils_ref_create_conf(sizeof(*bq), &rc_conf);
	} else {
		bq = malloc(sizeof(*bq));
	}

	if(!bq) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed to allocate the blocking queue!");
		goto err;
	}

	int failure = pthread_mutex_init(&bq->lock, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(conf->logger, "pthread_mutex_init: '%s'", strerror(failure));
		goto err_lock;
	}

	failure = pthread_cond_init(&bq->removed, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(conf->logger, "pthread_cond_init: '%s'", strerror(failure));
		goto err_removed;
	}

	failure = pthread_cond_init(&bq->added, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(conf->logger, "pthread_cond_init: '%s'", strerror(failure));
		goto err_added;
	}

	/*
		Note here, depending on if we use a comparator depends on what type of data structure we use.
		If the user provides a comparator for a priority blocking queue, then it is treated as such 
		and uses a binary heap. Otherwise, a normal list is used.
	*/
	if(conf->callbacks.comparators.item) {
		struct c_utils_heap_conf heap_conf =
		{
			.logger = conf->logger,
			.size =
			{
				.max = conf->size.max,
				.initial = conf->size.initial
			},
			.callbacks.destructors.item = conf->callbacks.destructors.item,
		};

		bq->data.heap = c_utils_heap_create_conf(conf->callbacks.comparators.item, &heap_conf);
	} else {
		struct c_utils_list_conf list_conf =
		{
			.logger = conf->logger,
			.callbacks.destructors.item = conf->callbacks.destructors.item,
			.size.max = conf->size.max
		};

		bq->data.list = c_utils_list_create_conf(&list_conf);
	}

	if(!bq->data.list || !bq->data.heap) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed while attempting to create heap or list...");
		goto err_data;
	}

	bq->shutdown = ATOMIC_VAR_INIT(false);
	bq->blocked = ATOMIC_VAR_INIT(0);
	bq->conf = *conf;

	return bq;

	err_data:
		pthread_cond_destroy(&bq->added);
	err_added:
		pthread_cond_destroy(&bq->removed);
	err_removed:
		pthread_mutex_destroy(&bq->lock);
	err_lock:
		if(conf->flags & C_UTILS_BLOCKING_QUEUE_RC_INSTANCE)
			c_utils_ref_destroy(bq);
		else
			free(bq);
	err:
		return NULL;
}

bool c_utils_blocking_queue_enqueue(struct c_utils_blocking_queue *bq, void *item, long long int timeout) {
	if(!bq)
		return false;

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	end = start;

	if(timeout != C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT) {
		end.tv_sec += timeout / 1000;
		end.tv_nsec += (timeout % 1000) * 1000000L;
	}

	atomic_fetch_add(&bq->blocked, 1);

	pthread_mutex_lock(&bq->lock);

	while(!atomic_load(&bq->shutdown) && !add_item(bq, item)) {
		int errcode;

		if (timeout == C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT)
			errcode = pthread_cond_wait(&bq->removed, &bq->lock);
		else
			errcode = pthread_cond_timedwait(&bq->removed, &bq->lock, &end);

		if (errcode) {
			if (errcode != ETIMEDOUT)
				C_UTILS_LOG_ERROR(bq->conf.logger, "%s: '%s'", timeout < 0 ? "pthread_cond_wait" : "pthread_cond_timedwait", strerror(errno));

			pthread_mutex_unlock(&bq->lock);
			atomic_fetch_sub(&bq->blocked, 1);

			return false;
		}
	}

	pthread_cond_signal(&bq->added);
	pthread_mutex_unlock(&bq->lock);
	atomic_fetch_sub(&bq->blocked, 1);

	return true;
}

/// Blocks until a new element is available or the amount of the time ellapses.
void *c_utils_blocking_queue_dequeue(struct c_utils_blocking_queue *bq, long long int timeout) {
	if(!bq)
		return NULL;

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	end = start;

	if(timeout != C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT) {
		end.tv_sec += timeout / 1000;
		end.tv_nsec += (timeout % 1000) * 1000000L;
	}

	atomic_fetch_add(&bq->blocked, 1);

	pthread_mutex_lock(&bq->lock);

	void *item;
	while(!atomic_load(&bq->shutdown) && !(item = take_item(bq))) {
		int errcode;

		if (timeout == C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT)
			errcode = pthread_cond_wait(&bq->added, &bq->lock);
		else
			errcode = pthread_cond_timedwait(&bq->added, &bq->lock, &end);

		if (errcode) {
			if (errcode != ETIMEDOUT)
				C_UTILS_LOG_ERROR(bq->conf.logger, "%s: '%s'", timeout < 0 ? "pthread_cond_wait" : "pthread_cond_timedwait", strerror(errno));

			pthread_mutex_unlock(&bq->lock);
			atomic_fetch_sub(&bq->blocked, 1);

			return false;
		}
	}

	pthread_cond_signal(&bq->removed);
	pthread_mutex_unlock(&bq->lock);
	atomic_fetch_sub(&bq->blocked, 1);

	return item;
}

bool c_utils_blocking_queue_remove_all(struct c_utils_blocking_queue *bq) {
	// TODO
	return false;
}

bool c_utils_blocking_queue_delete_all(struct c_utils_blocking_queue *bq) {
	// TODO
	return false;
}

/// Clear the queue, and optionally execute a callback on every item currently in the queue. I.E allows you to delete them.
bool c_utils_blocking_queue_clear(struct c_utils_blocking_queue *bq) {
	if(!bq)
		return false;
	
	atomic_fetch_add(&bq->blocked, 1);
	
	pthread_mutex_lock(&bq->lock);
	for (struct c_utils_node *curr = bq->head; curr; curr = bq->head) {
		if (del)
			del(curr->item);

		bq->head = curr->next;
		
		free(curr);
		
		bq->size--;
	}
	
	bq->tail = NULL;
	
	pthread_mutex_unlock(&bq->lock);
	atomic_fetch_sub(&bq->blocked, 1);
	
	return true;
}

size_t c_utils_blocking_queue_size(struct c_utils_blocking_queue *bq) {
	C_UTILS_ARG_CHECK(logger, 0, bq);
	
	return bq->size;
}

/// Clears the queue then destroys the queue. Will execute a callback on every item in the queue if not null.
bool c_utils_blocking_queue_destroy(struct c_utils_blocking_queue *bq) {
	C_UTILS_ARG_CHECK(logger, false, bq);

	pthread_mutex_lock(&bq->lock);
	atomic_store(&bq->shutdown, true);

	pthread_cond_broadcast(&bq->removed);
	pthread_cond_broadcast(&bq->added);

	pthread_mutex_unlock(&bq->lock);

	while (atomic_load(&bq->blocked) > 0)
		pthread_yield();

	pthread_cond_destroy(&bq->removed);
	pthread_cond_destroy(&bq->added);
	pthread_mutex_destroy(&bq->lock);

	free(bq);

	return true;
}