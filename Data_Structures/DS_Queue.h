#ifndef DS_QUEUE_H
#define DS_QUEUE_H

#include <DS_Helpers.h>
#include <MU_Hazard_Pointers.h>
#include <stdint.h>
#include <stdatomic.h>

/*
	DS_Queue is a lock-free, fast and minimal queue built on top of
	Maged M. Michael's Hazard Pointer implementation that solves the ABA
	problem. It is a very simple data structure, although also very powerful.

	It's operations are like any other queue, enqueue and dequeue, except
	with the guarantee to never block, nor suffer from priority inversions,
	deadlocks, livelocks, etc.
*/

typedef struct {
	DS_Node_t *head;
	DS_Node_t *tail;
	volatile size_t size;
} DS_Queue_t;


/*
 * Creates a new instance of the queue.
 * @return A new instance, or NULL if failure in allocating memory for the queue.
 */
DS_Queue_t *DS_Queue_create(void);

/*
 * Enqueue an item to the queue, with guarantee not to block. 
 * @param queue Instance of the queue.
 * @param data Data to enqueue.
 * @return true upon success, false if allocation fails for creating a node.
 */
bool DS_Queue_enqueue(DS_Queue_t *queue, void *data);

/*
 * Dequeue an item from the queue, with guarantee not to block.
 * @param queue Instance of the queue.
 * @return The data dequeued, or NULL if queue is NULL or if it is empty.
 */
void *DS_Queue_dequeue(DS_Queue_t *queue);

/*
 * Destroys the queue, calling the del deletion callback on each item in the
 * queue if specified.
 * @param queue Instance of the queue.
 * @param del Deletion callback to call on each item if specified.
 * @return true if successful, false if queue is NULL.
 */
bool DS_Queue_destroy(DS_Queue_t *queue, DS_delete_cb del);

#endif /* endif DS_QUEUE_H */