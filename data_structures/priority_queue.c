#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#include "priority_queue.h"
#include "../misc/argument_check.h"
#include "../misc/alloc_check.h"

struct c_utils_blocking_queue {
	/// A pointer to the head node.
	struct c_utils_node *head;
	/// A pointer to the tail node.
	struct c_utils_node *tail;
	/// To compare elements to determine priority.
	c_utils_comparator_cb compare;
	/// Size of the current queue, meaning amount of elements currently in the queue.
	_Atomic size_t size;
	/// The maximum size of the queue if it is bounded. If it is unbounded it is 0.
	size_t max_size;
	/// A new element may be added to the PBQueue.
	pthread_cond_t *not_full;
	/// A element has been added and may be removed from the PBQueue.
	pthread_cond_t *not_empty;
	/// The lock used to add or remove an element to/from the queue (respectively).
	pthread_mutex_t *lock;
	/// The amount of threads waiting.
	_Atomic size_t threads_waiting;
	/// Atomic flag for if it's being destroyed.
	_Atomic bool shutting_down;
};

static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./data_structures/logs/blocking_queue.log", "w", C_UTILS_LOG_LEVEL_ALL);

/* Static functions */

static inline bool add_as_head(struct c_utils_blocking_queue *queue, struct c_utils_node *node) {
	node->next = queue->head;
	queue->head = node;

	atomic_fetch_add(&queue->size, 1);

	return true;
}

static inline bool add_as_tail(struct c_utils_blocking_queue *queue, struct c_utils_node *node) {
	queue->tail->next = node;
	queue->tail = node;
	node->next = NULL;

	atomic_fetch_add(&queue->size, 1);

	return true;
}

static inline bool add_as_only(struct c_utils_blocking_queue *queue, struct c_utils_node *node) {
	node->next = NULL;
	queue->head = queue->tail = node;
	
	atomic_fetch_add(&queue->size, 1);

	return true;
}

static inline bool add_after(struct c_utils_blocking_queue *queue, struct c_utils_node *this_node, struct c_utils_node *prev) {
	this_node->next = prev->next;
	prev->next = this_node;
	
	atomic_fetch_add(&queue->size, 1);

	return true;
}

static bool add_item(struct c_utils_blocking_queue *queue, void *item) {
	struct c_utils_node *node;
	C_UTILS_ON_BAD_MALLOC(node, logger, sizeof(*node))
		return false;
	node->item = item;

	if (!queue->compare)
		return add_as_tail(queue, node);

	if (!queue->size)
		return add_as_only(queue, node);
	else if (queue->size == 1)
		// Checks to see if the item is of greater priority than the head of the queue.
		if (queue->compare(item, queue->head->item) > 0)
			return add_as_head(queue, node);
		else
			return add_as_tail(queue, node);
	else if (queue->compare(item, queue->head->item) > 0)
		return add_as_head(queue, node);
	else if (queue->compare(item, queue->tail->item) <= 0)
		return add_as_tail(queue, node);
	else
		for (struct c_utils_node *curr = queue->head, *prev = curr; curr; prev = curr, curr = curr->next)
			if (queue->compare(item, curr->item) > 0)
				return add_after(queue, node, prev);
			else if (!curr->next)
				return add_as_tail(queue, node);

	return true;
}

static void *take_item(struct c_utils_blocking_queue *queue) {
	struct c_utils_node *node = queue->head;
	if (!node) 
		return NULL;
	
	void *item = queue->head->item;
	queue->head = queue->head->next;
	
	free(node);
	atomic_fetch_sub(&queue->size, 1);
	
	return item;
}

/* End static functions */

/// Returns an initialized bounded queue of max size max_elements.
struct c_utils_blocking_queue *c_utils_blocking_queue_create(size_t max_elements, c_utils_comparator_cb compare) {	
	struct c_utils_blocking_queue *queue;
	C_UTILS_ON_BAD_CALLOC(queue, logger, sizeof(*queue))
		goto err;

	/// max_elements of 0 means unbounded.
	queue->max_size = max_elements;
	queue->head = queue->tail = NULL;

	C_UTILS_ON_BAD_MALLOC(queue->lock, logger, sizeof(*queue->lock))
		goto err_lock;

	int failure = pthread_mutex_init(queue->lock, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(failure));
		goto err_lock_init;
	}

	C_UTILS_ON_BAD_MALLOC(queue->not_full, logger, sizeof(*queue->not_full))
		goto err_not_full;

	failure = pthread_cond_init(queue->not_full, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(failure));
		goto err_not_full_init;
	}

	C_UTILS_ON_BAD_MALLOC(queue->not_empty, logger, sizeof(*queue->not_empty))
		goto err_not_empty;

	failure = pthread_cond_init(queue->not_empty, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(failure));
		goto err_not_empty_init;
	}

	queue->compare = compare;
	queue->shutting_down = ATOMIC_VAR_INIT(false);
	queue->threads_waiting = ATOMIC_VAR_INIT(0);
	queue->size = ATOMIC_VAR_INIT(0);
	
	return queue;

	err_not_empty_init:
		free(queue->not_empty);
	err_not_empty:
		pthread_cond_destroy(queue->not_full);
	err_not_full_init:
		free(queue->not_full);
	err_not_full:
		pthread_mutex_destroy(queue->lock);
	err_lock_init:
		free(queue->lock);
	err_lock:
		free(queue);
	err:
		return NULL;
}

/// Blocks until either another element can be inserted or the time ellapses.
bool c_utils_blocking_queue_enqueue(struct c_utils_blocking_queue *queue, void *item, long long int timeout) {
	C_UTILS_ARG_CHECK(logger, false, queue);

	atomic_fetch_add(&queue->threads_waiting, 1);
	
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;
	
	pthread_mutex_lock(queue->lock);
	if (queue->max_size) {
		while (atomic_load(&queue->shutting_down) == false && atomic_load(&queue->size) == queue->max_size) {
			int errcode;
			if (timeout < 0)  
				errcode = pthread_cond_wait(queue->not_full, queue->lock);
			 else   
				errcode = pthread_cond_timedwait(queue->not_full, queue->lock, &ts);
			
			if (errcode) {
				if (errcode != ETIMEDOUT)
					C_UTILS_LOG_ERROR(logger, "%s: '%s'", timeout < 0 ? "pthread_cond_wait" : "pthread_cond_timedwait", strerror(errno));
				
				pthread_mutex_unlock(queue->lock);
				atomic_fetch_sub(&queue->threads_waiting, 1);
				return false;
			}
		}
	}

	if (atomic_load(&queue->shutting_down) == true) {
		pthread_mutex_unlock(queue->lock);
		atomic_fetch_sub(&queue->threads_waiting, 1);
		return false;
	}
	
	add_item(queue, item);
	
	pthread_cond_signal(queue->not_empty);
	pthread_mutex_unlock(queue->lock);
	atomic_fetch_sub(&queue->threads_waiting, 1);
	
	return 1;
}

/// Blocks until a new element is available or the amount of the time ellapses.
void *c_utils_blocking_queue_dequeue(struct c_utils_blocking_queue *queue, long long int timeout) {
	C_UTILS_ARG_CHECK(logger, NULL, queue);
	
	atomic_fetch_add(&queue->threads_waiting, 1);
	
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;
	
	pthread_mutex_lock(queue->lock);
	while (atomic_load(&queue->shutting_down) == false && atomic_load(&queue->size) == 0) {
		int errcode;
		if (timeout < 0)
			errcode = pthread_cond_wait(queue->not_empty, queue->lock);
		 else
			errcode = pthread_cond_timedwait(queue->not_empty, queue->lock, &ts);
		
		if (errcode) {
			if (errcode != ETIMEDOUT)
				C_UTILS_LOG_ERROR(logger, "%s: '%s'", timeout < 0 ? "pthread_cond_wait" : "pthread_cond_timedwait", strerror(errno));
			
			pthread_mutex_unlock(queue->lock);
			atomic_fetch_sub(&queue->threads_waiting, 1);
			
			return false;
		}
	}

	void *item = take_item(queue);
	
	pthread_cond_signal(queue->not_full);
	pthread_mutex_unlock(queue->lock);
	atomic_fetch_sub(&queue->threads_waiting, 1);
	
	return item;
}

/// Clear the queue, and optionally execute a callback on every item currently in the queue. I.E allows you to delete them.
bool c_utils_blocking_queue_clear(struct c_utils_blocking_queue *queue, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, false, queue);
	
	atomic_fetch_add(&queue->threads_waiting, 1);
	
	pthread_mutex_lock(queue->lock);
	for (struct c_utils_node *curr = queue->head; curr; curr = queue->head) {
		if (del)
			del(curr->item);

		queue->head = curr->next;
		
		free(curr);
		
		atomic_fetch_sub(&queue->size, 1);
	}
	
	queue->tail = NULL;
	
	pthread_mutex_unlock(queue->lock);
	atomic_fetch_sub(&queue->threads_waiting, 1);
	
	return true;
}

size_t c_utils_blocking_queue_size(struct c_utils_blocking_queue *queue) {
	C_UTILS_ARG_CHECK(logger, 0, queue);
	
	return atomic_load(&queue->size);
}

/// Clears the queue then destroys the queue. Will execute a callback on every item in the queue if not null.
bool c_utils_blocking_queue_destroy(struct c_utils_blocking_queue *queue, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, false, queue);
	
	c_utils_blocking_queue_clear(queue, del);
	
	pthread_mutex_lock(queue->lock);
	atomic_store(&queue->shutting_down, true);
	
	pthread_cond_broadcast(queue->not_full);
	pthread_cond_broadcast(queue->not_empty);
	
	pthread_mutex_unlock(queue->lock);
	
	while (atomic_load(&queue->threads_waiting) > 0)
		pthread_yield();
	
	pthread_cond_destroy(queue->not_full);
	pthread_cond_destroy(queue->not_empty);
	pthread_mutex_destroy(queue->lock);

	free(queue->not_full);
	free(queue->not_empty);
	free(queue->lock);
	free(queue);
	
	return true;
}