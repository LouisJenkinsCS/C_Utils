#ifndef C_UTILS_BLOCKING_QUEUE_H
#define C_UTILS_BLOCKING_QUEUE_H

#include "helpers.h"

#define C_UTILS_BLOCKING_QUEUE_RC_INSTANCE 1 << 0

#define C_UTILS_BLOCKING_QUEUE_RC_ITEM 1 << 1

#define C_UTILS_BLOCKING_QUEUE_DESTROY_ON_DELETE 1 << 2

#define C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT -1

/*
	c_utils_blocking_queue is a minimal blocking queue which features not only a way to
	create a bounded or unbounded queue, but also can be used as a prioritized blocking queue
	if a comparator is specified.

	If it is a normal blocking queue, all insertions will be appended to the tail as a normal queue, and
	the caller will block up to the specified timeout or until there is room to insert (only applicable if
	the queue is bounded). If instead it is a prioritized blocking queue, then insertions will be done
	using the comparator to add
	Items inserted are inserted in a sorted order using the passed comparator, and if there is none,
	it will be appened to the tail of the queue. This queue supports blocking and non-blocking operations,
	which can be configured using the timeout parameter. A timeout of -1 specifies an infinite length of time
	to wait, and a timeout of 0 will immediately fail or succeed if the queue's operation is available.
*/	
struct c_utils_blocking_queue;

struct c_utils_blocking_queue_conf {
	int flags;
	struct {
		struct {
			int (*item)(const void *, const void *);
		} comparators;
		struct {
			void (*item)(void *);
		} destructors;
	} callbacks;
	struct {
		size_t initial;
		size_t max;
	} size;
	struct c_utils_logger *logger;
};

#ifdef NO_C_UTILS_PREFIX
/*
	Typedef
*/
typedef struct c_utils_blocking_queue blocking_queue_t;
typedef struct c_utils_blocking_queue_conf blocking_queue_conf_t;

/*
	Functions
*/
#define blocking_queue_create(...) c_utils_blocking_queue_create(__VA_ARGS__)
#define blocking_queue_enqueue(...) c_utils_blocking_queue_enqueue(__VA_ARGS__)
#define blocking_queue_dequeue(...) c_utils_blocking_queue_dequeue(__VA_ARGS__)
#define blocking_queue_clear(...) c_utils_blocking_queue_clear(__VA_ARGS__)
#define blocking_queue_size(...) c_utils_blocking_queue_size(__VA_ARGS__)
#define blocking_queue_destroy(...) c_utils_blocking_queue_destroy(__VA_ARGS__)
#endif



/**
* Creates a blocking queue with default configurations. This will cause it to act like a normal
* blockign queue, without a priority.
*/
struct c_utils_blocking_queue *c_utils_blocking_queue_create();

/**
* Creates and configures the blocking queue to the passed settings. If a comparator is passed, it may
* also act as prioritized blocking queue, and if max size is specified it will be bounded.
*/
struct c_utils_blocking_queue *c_utils_blocking_queue_create_conf(struct c_utils_blocking_queue_conf *conf);

/**
 * Enqueue an item to the queue, waiting until timeout if the queue is bounded and full.
 * @param queue Instance of the queue.
 * @param item The item to enqueue.
 * @param timeout Maximum timeout to wait until completion.
 * @return true if successful, false if queue is NULL, or timeout ellapses before succeeding.
 */
bool c_utils_blocking_queue_enqueue(struct c_utils_blocking_queue *queue, void *item, long long int timeout);

/**
 * Dequeue an item from the queue, waiting until timeout if the queue is bounded and full.
 * @param queue Instance of the queue.
 * @param timeout Maximum timeout to wait until completion.
 * @return The item dequeued, or NULL, if the queue is NULL or is empty.
 */
void *c_utils_blocking_queue_dequeue(struct c_utils_blocking_queue *queue, long long int timeout);

/**
 * Clears the queue of all items, calling the del deletion callback on any items
 * if specified.
 * @param queue Instance of the queue.
 * @param del Deletion callback to be called on each item if specified.
 * @return true if successful, or false if queue is NULL.
 */
bool c_utils_blocking_queue_remove_all(struct c_utils_blocking_queue *queue);

bool c_utils_blocking_queue_delete_all(struct c_utils_blocking_queue *queue);

/**
 * Returns the size of the queue.
 * @param queue Instance of the queue.
 * @return The size of the queue, or 0 if empty or queue is NULL.
 */
size_t c_utils_blocking_queue_size(struct c_utils_blocking_queue *queue);

/**
 * Destroys the instance of the queue, waking up all threads waiting for to enqueue/dequeue.
 * If the del deletion callback is specified, it is called on each item in the list.
 * @param queue Instance of the queue.
 * @param del Deletion callback to be called on each item if specified.
 * @return true if successful, false if queue is NULL.
 */
bool c_utils_blocking_queue_destroy(struct c_utils_blocking_queue *queue);

#endif /* C_UTILS_BLOCKING_QUEUE_H */