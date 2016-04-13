#ifndef C_UTILS_BLOCKING_QUEUE_H
#define C_UTILS_BLOCKING_QUEUE_H

#include "helpers.h"

#define C_UTILS_BLOCKING_QUEUE_RC_INSTANCE 1 << 0

#define C_UTILS_BLOCKING_QUEUE_RC_ITEM 1 << 1

#define C_UTILS_BLOCKING_QUEUE_DELETE_ON_DESTROY 1 << 2

#define C_UTILS_BLOCKING_QUEUE_NO_TIMEOUT -1

/*
	blocking_queue_t is a configurable blocking queue, with the ability to serve as a 
	prioritized blocking queue. It supports the ability shutdown the blocking queue and
	wake up threads, as well as prevent more threads from attempting to block on it. The
	heap is also optimized to treat non prioritized items vs prioritized items, whereas
	non prioritized use a standard list, and prioritized uses a binary heap.

	The priority queue provides an abstraction on top of both data structures to allow
	for blocking operations ideal for producer/consumer scenarios, and in the future there
	likely will be support for user-defined data structures which both consume and produce
	items.

	The blocking queue can also, to an extent, function as a normal queue or priority queue
	by inducing 0-millisecond timeouts.
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
#define blocking_queue_create_conf(...) c_utils_blocking_queue_create_conf(__VA_ARGS__)
#define blocking_queue_enqueue(...) c_utils_blocking_queue_enqueue(__VA_ARGS__)
#define blocking_queue_dequeue(...) c_utils_blocking_queue_dequeue(__VA_ARGS__)
#define blocking_queue_remove_all(...) c_utils_blocking_queue_remove_all(__VA_ARGS__)
#define blocking_queue_delete_all(...) c_utils_blocking_queue_delete_all(__VA_ARGS__)
#define blocking_queue_size(...) c_utils_blocking_queue_size(__VA_ARGS__)
#define blocking_queue_activate(...) c_utils_blocking_queue_activate(__VA_ARGS__)
#define blocking_queue_shutdown(...) c_utils_blocking_queue_shutdown(__VA_ARGS__)
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
void c_utils_blocking_queue_remove_all(struct c_utils_blocking_queue *queue);

void c_utils_blocking_queue_delete_all(struct c_utils_blocking_queue *queue);

/**
 * Returns the size of the queue.
 * @param queue Instance of the queue.
 * @return The size of the queue, or 0 if empty or queue is NULL.
 */
size_t c_utils_blocking_queue_size(struct c_utils_blocking_queue *queue);

void c_utils_blocking_queue_shutdown(struct c_utils_blocking_queue *queue);

void c_utils_blocking_queue_activate(struct c_utils_blocking_queue *queue);

/**
 * Destroys the instance of the queue, waking up all threads waiting for to enqueue/dequeue.
 * If the del deletion callback is specified, it is called on each item in the list.
 * @param queue Instance of the queue.
 * @param del Deletion callback to be called on each item if specified.
 * @return true if successful, false if queue is NULL.
 */
void c_utils_blocking_queue_destroy(struct c_utils_blocking_queue *queue);

#endif /* C_UTILS_BLOCKING_QUEUE_H */