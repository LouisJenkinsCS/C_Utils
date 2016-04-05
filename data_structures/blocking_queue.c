#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

#include "blocking_queue.h"
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
	volatile size_t size;
	/// The maximum size of the queue if it is bounded. If it is unbounded it is 0.
	size_t max_size;
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
};

static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./data_structures/logs/blocking_queue.log", "w", C_UTILS_LOG_LEVEL_ALL);

/* Static functions */

static inline bool add_as_head(struct c_utils_blocking_queue *bq, struct c_utils_node *node) {
	node->next = bq->head;
	bq->head = node;

	bq->size++;

	return true;
}

static inline bool add_as_tail(struct c_utils_blocking_queue *bq, struct c_utils_node *node) {
	bq->tail->next = node;
	bq->tail = node;
	node->next = NULL;

	bq->size++;

	return true;
}

static inline bool add_as_only(struct c_utils_blocking_queue *bq, struct c_utils_node *node) {
	node->next = NULL;
	bq->head = bq->tail = node;
	
	bq->size++;

	return true;
}

static inline bool add_after(struct c_utils_blocking_queue *bq, struct c_utils_node *this_node, struct c_utils_node *prev) {
	this_node->next = prev->next;
	prev->next = this_node;

	bq->size++;

	return true;
}

static bool add_item(struct c_utils_blocking_queue *bq, void *item) {
	struct c_utils_node *node;
	C_UTILS_ON_BAD_MALLOC(node, logger, sizeof(*node))
		return false;
	node->item = item;

	if (!bq->compare)
		return add_as_tail(bq, node);

	if (!bq->size)
		return add_as_only(bq, node);
	else if (bq->size == 1)
		// Checks to see if the item is of greater priority than the head of the queue.
		if (bq->compare(item, bq->head->item) > 0)
			return add_as_head(bq, node);
		else
			return add_as_tail(bq, node);
	else if (bq->compare(item, bq->head->item) > 0)
		return add_as_head(bq, node);
	else if (bq->compare(item, bq->tail->item) <= 0)
		return add_as_tail(bq, node);
	else
		for (struct c_utils_node *curr = bq->head, *prev = curr; curr; prev = curr, curr = curr->next)
			if (bq->compare(item, curr->item) > 0)
				return add_after(bq, node, prev);
			else if (!curr->next)
				return add_as_tail(bq, node);

	return true;
}

static void *take_item(struct c_utils_blocking_queue *bq) {
	struct c_utils_node *node = bq->head;
	if (!node) 
		return NULL;
	
	void *item = bq->head->item;
	bq->head = bq->head->next;
	
	free(node);
	bq->size--;
	
	return item;
}

/* End static functions */

/// Returns an initialized bounded queue of max size max_elements.
struct c_utils_blocking_queue *c_utils_blocking_queue_create(size_t max_elements, c_utils_comparator_cb compare) {
	struct c_utils_blocking_queue *bq;
	C_UTILS_ON_BAD_CALLOC(bq, logger, sizeof(*bq))
		goto err;

	/// max_elements of 0 means unbounded.
	bq->max_size = max_elements;
	bq->head = bq->tail = NULL;

	int failure = pthread_mutex_init(&bq->lock, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(failure));
		goto err_lock;
	}

	failure = pthread_cond_init(&bq->removed, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(failure));
		goto err_removed;
	}

	failure = pthread_cond_init(&bq->added, NULL);
	if (failure) {
		C_UTILS_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(failure));
		goto err_added;
	}

	bq->compare = compare;
	bq->shutdown = ATOMIC_VAR_INIT(false);
	bq->blocked = ATOMIC_VAR_INIT(0);
	bq->size = ATOMIC_VAR_INIT(0);
	
	return bq;

	err_added:
		pthread_cond_destroy(&bq->removed);
	err_removed:
		pthread_mutex_destroy(&bq->lock);
	err_lock:
		free(bq);
	err:
		return NULL;
}

/// Blocks until either another element can be inserted or the time ellapses.
bool c_utils_blocking_queue_enqueue(struct c_utils_blocking_queue *bq, void *item, long long int timeout) {
	if(!bq)
		return false;

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;
	bool retval = false;

	atomic_fetch_add(&bq->blocked, 1);

	pthread_mutex_lock(&bq->lock);
	if (bq->max_size) {
		while (!atomic_load(&bq->shutdown) && bq->size == bq->max_size) {
			int errcode;
			if (timeout < 0) 
				errcode = pthread_cond_wait(&bq->removed, &bq->lock);
			else
				errcode = pthread_cond_timedwait(&bq->removed, &bq->lock, &ts);

			if (errcode) {
				if (errcode != ETIMEDOUT)
					C_UTILS_LOG_ERROR(logger, "%s: '%s'", timeout < 0 ? "pthread_cond_wait" : "pthread_cond_timedwait", strerror(errno));

				pthread_mutex_unlock(&bq->lock);
				atomic_fetch_sub(&bq->blocked, 1);

				return false;
			}
		}
	}

	if (!atomic_load(&bq->shutdown)) {
		add_item(bq, item);
		pthread_cond_signal(&bq->added);
		
		retval = true;
	}
	
	
	pthread_mutex_unlock(&bq->lock);
	atomic_fetch_sub(&bq->blocked, 1);
	
	return retval;
}

/// Blocks until a new element is available or the amount of the time ellapses.
void *c_utils_blocking_queue_dequeue(struct c_utils_blocking_queue *bq, long long int timeout) {
	if(!bq)
		return NULL;
	
	atomic_fetch_add(&bq->blocked, 1);
	
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;
	void *item = NULL;
	
	pthread_mutex_lock(&bq->lock);
	while (atomic_load(&bq->shutdown) == false && !bq->size) {
		int errcode;
		if (timeout < 0)
			errcode = pthread_cond_wait(&bq->added, &bq->lock);
		else
			errcode = pthread_cond_timedwait(&bq->added, &bq->lock, &ts);

		if (errcode) {
			if (errcode != ETIMEDOUT)
				C_UTILS_LOG_ERROR(logger, "%s: '%s'", timeout < 0 ? "pthread_cond_wait" : "pthread_cond_timedwait", strerror(errno));

			pthread_mutex_unlock(&bq->lock);
			atomic_fetch_sub(&bq->blocked, 1);

			return NULL;
		}
	}

	if(!atomic_load(&bq->shutdown)) {
		item = take_item(bq);
		pthread_cond_signal(&bq->removed);
	}

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
bool c_utils_blocking_queue_clear(struct c_utils_blocking_queue *bq, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, false, bq);
	
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