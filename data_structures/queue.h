#ifndef C_UTILS_QUEUE_H
#define C_UTILS_QUEUE_H

#include <stdbool.h>

#include "helpers.h"

/*
	c_utils_queue is a lock-free, fast and minimal queue built on top of
	Maged M. Michael's Hazard Pointer implementation that solves the ABA
	problem. It is a very simple data structure, although also very powerful.

	It's operations are like any other queue, enqueue and dequeue, except
	with the guarantee to never block, nor suffer from priority inversions,
	deadlocks, livelocks, etc.
*/
struct c_utils_queue;

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_queue queue_t;

/*
	Functions
*/
#define queue_create(...) c_utils_queue_create(__VA_ARGS__)
#define queue_enqueue(...) c_utils_queue_enqueue(__VA_ARGS__)
#define queue_dequeue(...) c_utils_queue_dequeue(__VA_ARGS__)
#define queue_destroy(...) c_utils_queue_destroy(__VA_ARGS__)
#endif

/*
 * Creates a new instance of the queue.
 * @return A new instance, or NULL if failure in allocating memory for the queue.
 */
struct c_utils_queue *c_utils_queue_create(void);

/*
 * Enqueue an item to the queue, with guarantee not to block. 
 * @param queue Instance of the queue.
 * @param data Data to enqueue.
 * @return true upon success, false if allocation fails for creating a node.
 */
bool c_utils_queue_enqueue(struct c_utils_queue *queue, void *data);

/*
 * Dequeue an item from the queue, with guarantee not to block.
 * @param queue Instance of the queue.
 * @return The data dequeued, or NULL if queue is NULL or if it is empty.
 */
void *c_utils_queue_dequeue(struct c_utils_queue *queue);

/*
 * Destroys the queue, calling the del deletion callback on each item in the
 * queue if specified.
 * @param queue Instance of the queue.
 * @param del Deletion callback to call on each item if specified.
 * @return true if successful, false if queue is NULL.
 */
bool c_utils_queue_destroy(struct c_utils_queue *queue, c_utils_delete_cb del);

#endif /* endif C_UTILS_QUEUE_H */