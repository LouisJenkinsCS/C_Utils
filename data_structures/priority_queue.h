#ifndef C_UTILS_PRIORITY_QUEUE
#define C_UTILS_PRIORITY_QUEUE

#include "helpers.h"

/*
	c_utils_priority_queue is a minimal priority blocking queue which features not only a way to 
	create a bounded or unbounded queue, but also be used as a normal blocking queue
	if no comparator callback is used, or even as a normal queue if a timeout of 0 is 
	used each time. 

	Items inserted are inserted in a sorted order using the passed comparator, and if there is none,
	it will be appened to the tail of the queue. This queue supports blocking and non-blocking operations,
	which can be configured using the timeout parameter. A timeout of -1 specifies an infinite length of time
	to wait, and a timeout of 0 will immediately fail or succeed if the queue's operation is available.

	The Priority Blocking Queue will wake up all threads blocking before destroying itself,
	to prevent permanent deadlocks.
*/	
struct c_utils_priority_queue;

#ifdef NO_C_UTILS_PREFIX
/*
	Typedef
*/
typedef struct c_utils_priority_queue priority_queue_t;

/*
	Functions
*/
#define priority_queue_create(...) c_utils_priority_queue_create(__VA_ARGS__)
#define priority_queue_enqueue(...) c_utils_priority_queue_enqueue(__VA_ARGS__)
#define priority_queue_dequeue(...) c_utils_priority_queue_dequeue(__VA_ARGS__)
#define priority_queue_clear(...) c_utils_priority_queue_clear(__VA_ARGS__)
#define priority_queue_size(...) c_utils_priority_queue_size(__VA_ARGS__)
#define priority_queue_destroy(...) c_utils_priority_queue_destroy(__VA_ARGS__)
#endif





/**
 * Create a new instance of the priority blocking queue, with the option of having an unbounded or
 * bounded queue (if max_elements == 0). The comparator used is defined during creation and cannot be changed
 * later.
 *
 * If compare is left NULL, it will operate as a normal blocking queue. 
 * @param max_elements Maximum elements if bounded, unbounded if left at 0.
 * @param compare Comparator callback sort the queue.
 * @return A bounded or unbounded priority blocking queue, or NULL if failure in allocation.
 */
struct c_utils_priority_queue *c_utils_priority_queue_create(size_t max_elements, c_utils_comparator_cb compare);

/**
 * Enqueue an item to the queue, waiting until timeout if the queue is bounded and full.
 * @param queue Instance of the queue.
 * @param item The item to enqueue.
 * @param timeout Maximum timeout to wait until completion.
 * @return true if successful, false if queue is NULL, or timeout ellapses before succeeding.
 */
bool c_utils_priority_queue_enqueue(struct c_utils_priority_queue *queue, void *item, long long int timeout);

/**
 * Dequeue an item from the queue, waiting until timeout if the queue is bounded and full.
 * @param queue Instance of the queue.
 * @param timeout Maximum timeout to wait until completion.
 * @return The item dequeued, or NULL, if the queue is NULL or is empty.
 */
void *c_utils_priority_queue_dequeue(struct c_utils_priority_queue *queue, long long int timeout);

/**
 * Clears the queue of all items, calling the del deletion callback on any items
 * if specified. 
 * @param queue Instance of the queue.
 * @param del Deletion callback to be called on each item if specified.
 * @return true if successful, or false if queue is NULL.
 */
bool c_utils_priority_queue_clear(struct c_utils_priority_queue *queue, c_utils_delete_cb del);

/**
 * Returns the size of the queue.
 * @param queue Instance of the queue.
 * @return The size of the queue, or 0 if empty or queue is NULL.
 */
size_t c_utils_priority_queue_size(struct c_utils_priority_queue *queue);

/**
 * Destroys the instance of the queue, waking up all threads waiting for to enqueue/dequeue.
 * If the del deletion callback is specified, it is called on each item in the list.
 * @param queue Instance of the queue.
 * @param del Deletion callback to be called on each item if specified.
 * @return true if successful, false if queue is NULL.
 */
bool c_utils_priority_queue_destroy(struct c_utils_priority_queue *queue, c_utils_delete_cb del);

#endif /* C_UTILS_PRIORITY_QUEUE */