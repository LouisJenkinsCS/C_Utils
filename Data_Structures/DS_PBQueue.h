#include <DS_Helpers.h>
#include <pthread.h>
#include <stdatomic.h>

/*
 * PBQueue is a minimal priority blocking queue, which will insert elements sorted
 * in ascending order based on priority. The priority is determined by the comparator passed to it.
 * It should be noted that this can be doubled as a normal blocking queue by having
 * a comparator which just returns 0, like such.
 * 
 * int comparator(void *item_one, void *item_two){
 *      return 0;
 * }
 * 
 * Since the priority blocking queue will detect whether it should be at the tail of the queue,
 * doing so would insert it at the end with O(1) insertion. It should also be noted that
 * if an item is not the lowest priority, the insertion is O(n).
 * 
 * Alternatively, this can be used a regular blocking queue by calling the Timed_Enqueue
 * and Timed_Dequeue with 0 seconds, causing it to timeout immediately if the queue is
 * full/empty (respectively). A combination of the two could turn it into a normal
 * queue with it's own quirks.
 * 
 * Lastly, this Priority Blocking Queue can either be a bounded or unbounded queue
 * depending on which constructor used to create it.
 */

typedef struct {
	/// A pointer to the head node.
	DS_Node_t *head;
	/// A pointer to the tail node.
	DS_Node_t *tail;
	/// To compare elements to determine priority.
	DS_comparator_cb compare;
	/// Size of the current queue, meaning amount of elements currently in the queue.
	_Atomic size_t size;
	/// The maximum size of the queue if it is bounded. If it is unbounded it is 0.
	size_t max_size;
	/// A new element may be added to the PBQueue.
	pthread_cond_t *not_full;
	/// A element has been added and may be removed from the PBQueue.
	pthread_cond_t *not_empty;
	/// The lock used to add or remove an element to/from the queue (respectively).
	pthread_mutex_t *manipulating_queue;
	/// The amount of threads waiting.
	_Atomic size_t threads_waiting;
	/// Atomic flag for if it's being destroyed.
	_Atomic bool shutting_down;
} DS_PBQueue_t;

/**
 * 
 * @param max_elements
 * @param compare
 * @return 
 */
DS_PBQueue_t *DS_PBQueue_create(size_t max_elements, DS_comparator_cb compare);

/**
 * 
 * @param queue
 * @param item
 * @param timeout
 * @return 
 */
bool DS_PBQueue_enqueue(DS_PBQueue_t *queue, void *item, long long int timeout);

/**
 * 
 * @param queue
 * @param timeout
 * @return 
 */
void *DS_PBQueue_dequeue(DS_PBQueue_t *queue, long long int timeout);

/**
 * 
 * @param queue
 * @param del
 * @return 
 */
bool DS_PBQueue_clear(DS_PBQueue_t *queue, DS_delete_cb del);

/**
 * 
 * @param queue
 * @return 
 */
size_t DS_PBQueue_size(DS_PBQueue_t *queue);

/**
 * 
 * @param queue
 * @param del
 * @return 
 */
bool DS_PBQueue_destroy(DS_PBQueue_t *queue, DS_delete_cb del);