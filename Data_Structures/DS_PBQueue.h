#include <DS_Helpers.h>
#include <pthread.h>
#include <stdatomic.h>

#ifdef C_UTILS_USE_POSIX_STD
#define pbqueue_t DS_PBQueue_t
#define pbqueue_create(...) DS_PBQueue_create(__VA_ARGS__)
#define pbqueue_enqueue(...) DS_PBQueue_enqueue(__VA_ARGS__)
#define pbqueue_dequeue(...) DS_PBQueue_dequeue(__VA_ARGS__)
#define pbqueue_clear(...) DS_PBQueue_clear(__VA_ARGS__)
#define pbqueue_size(...) DS_PBQueue_size(__VA_ARGS__)
#define pbqueue_destroy(...) DS_PBQueue_destroy(__VA_ARGS__)
#endif

/*
	PBQueue is a minimal priority blocking queue which features not only a way to 
	create a bounded or unbounded queue, but also be used as a normal blocking queue
	if no comparator callback is used, or even as a normal queue if a timeout of 0 is 
	used each time. 

	The Priority Blocking Queue will wake up all threads blocking before destroying itself,
	to prevent permanent deadlocks.
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
 * Create a new instance of the priority blocking queue, with the option of having an unbounded or
 * bounded queue (if max_elements == 0). The comparator used is defined during creation and cannot be changed
 * later.
 *
 * If compare is left NULL, it will operate as a normal blocking queue. 
 * @param max_elements Maximum elements if bounded, unbounded if left at 0.
 * @param compare Comparator callback sort the queue.
 * @return A bounded or unbounded priority blocking queue, or NULL if failure in allocation.
 */
DS_PBQueue_t *DS_PBQueue_create(size_t max_elements, DS_comparator_cb compare);

/**
 * Enqueue an item to the queue, waiting until timeout if the queue is bounded and full.
 * @param queue Instance of the queue.
 * @param item The item to enqueue.
 * @param timeout Maximum timeout to wait until completion.
 * @return true if successful, false if queue is NULL, or timeout ellapses before succeeding.
 */
bool DS_PBQueue_enqueue(DS_PBQueue_t *queue, void *item, long long int timeout);

/**
 * Dequeue an item from the queue, waiting until timeout if the queue is bounded and full.
 * @param queue Instance of the queue.
 * @param timeout Maximum timeout to wait until completion.
 * @return The item dequeued, or NULL, if the queue is NULL or is empty.
 */
void *DS_PBQueue_dequeue(DS_PBQueue_t *queue, long long int timeout);

/**
 * Clears the queue of all items, calling the del deletion callback on any items
 * if specified. 
 * @param queue Instance of the queue.
 * @param del Deletion callback to be called on each item if specified.
 * @return true if successful, or false if queue is NULL.
 */
bool DS_PBQueue_clear(DS_PBQueue_t *queue, DS_delete_cb del);

/**
 * Returns the size of the queue.
 * @param queue Instance of the queue.
 * @return The size of the queue, or 0 if empty or queue is NULL.
 */
size_t DS_PBQueue_size(DS_PBQueue_t *queue);

/**
 * Destroys the instance of the queue, waking up all threads waiting for to enqueue/dequeue.
 * If the del deletion callback is specified, it is called on each item in the list.
 * @param queue Instance of the queue.
 * @param del Deletion callback to be called on each item if specified.
 * @return true if successful, false if queue is NULL.
 */
bool DS_PBQueue_destroy(DS_PBQueue_t *queue, DS_delete_cb del);