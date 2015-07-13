#include <pthread.h>

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

typedef struct PBQueue PBQueue;

typedef struct PBQ_Node PBQ_Node;

typedef enum PBQ_Type PBQ_Type;

/// Easier to type comparator callback typedef.
typedef int (*compare_elements)(void *item_one, void *item_two);

struct PBQ_Node{
	/// Pointer to the next node.
	PBQ_Node *next;
	/// The user's item added to the queue.
	void *item;
};

struct PBQueue {
	/// A pointer to the head node.
	PBQ_Node *head;
	/// A pointer to the tail node.
	PBQ_Node *tail;
	/// To compare elements to determine priority.
	compare_elements comparator;
	/// Size of the current queue, meaning amount of elements currently in the queue.
	volatile size_t size;
	/// The maximum size of the queue if it is bounded. If it is unbounded it is 0.
	size_t max_size;
	/// A new element may be added to the PBQueue.
	pthread_cond_t *is_not_full;
	/// A element has been added and may be removed from the PBQueue.
	pthread_cond_t *is_not_empty;
	/// The lock used to add or remove an element to/from the queue (respectively).
	pthread_mutex_t *adding_or_removing_elements;
};

/**
 * Returns an initialized bounded queue of max size max_elements.
 * @param max_elements The maximum size of the bounded queue.
 * @param comparator Callback used to compare elements. Cannot be NULL!
 * @return A bounded priority blocking queue.
 */
PBQueue *PBQueue_Create_Bounded(size_t max_elements, compare_elements comparator);

/**
 * Returns an initialized unbounded queue.
 * @param comparator Callback used to compare elements. Cannot be NULL!
 * @return A unbounded priority blocking queue.
 */
PBQueue *PBQueue_Create_Unbounded(compare_elements comparator);

/**
 * Will attempt to add an element to the queue. If it is bounded, it will block
 * until it is no longer full. Note: It is recommended you use the Timed_Enqueue
 * if your queue is bounded, due to the fact that it will block indefinitely until
 * an element is removed.
 * @param queue The priority queue to add an element to.
 * @param item The item to add to the priority queue.
 * @return 1 if successful.
 */
int PBQueue_Enqueue(PBQueue *queue, void *item);

/**
 * Attempts to add an element to the queue and will return early if the time elapses.
 * This is the recommended way to deal with bounded queues, as it can timeout early,
 * allowing you to avoid an infinite deadlock.
 * @param queue Priority blocking queue to add an element to.
 * @param item Element to be added to the priority blocking queue.
 * @param seconds Maximum time spent waiting to add an element.
 * @return 1 if successful, 0 if timed out.
 */
int PBQueue_Timed_Enqueue(PBQueue *queue, void *item, unsigned int seconds);

/**
 * Attempts to obtain an item from the priority queue, blocking until one becomes available.
 * It should be noted that Timed_Dequeue is recommended for general, non-specific use,
 * as this will cause the thread to block indefinitely until it can obtain an element.
 * @param queue Priority blocking queue to obtain an element from.
 * @return The head item of the priority blocking queue.
 */
void *PBQueue_Dequeue(PBQueue *queue);

/**
 * Attempts to obtain an item from the priority queue, blocking until one becomes available,
 * or the amount of time elapses. This is the overall recommended way, as it allows
 * threads to return early.
 * @param queue Priority blocking queue to obtain an element from.
 * @param seconds Maximum amount of time to attempt waiting to retrieve an item.
 * @return The head item of the queue if successful, or NULL if it times out.
 */
void *PBQueue_Timed_Dequeue(PBQueue *queue, unsigned int seconds);

/**
 * Returns whether the queue is empty or not. Note: Just because the queue is empty
 * one second, doesn't mean it will also be the next. 
 * @param queue Priority blocking queue to obtain an element from.
 * @return 1 if it is empty at this moment, or 0 if not.
 */
int PBQueue_Is_Empty(PBQueue *queue);

/**
 * Returns whether the queue is full or not. Note: Just because the queue is full
 * one second, doesn't mean it will also be the next.
 * @param queue Priority blocking queue to obtain an element from.
 * @return 1 if it is full at this moment, or 0 if not.
 */
int PBQueue_Is_Full(PBQueue *queue);

/**
 * Returns the current size of the queue. Note: Just because the queue's size is
 * returned, does not mean that by the time you act on the queue size, it will be the
 * same if other threads are manipulating the queue.
 * @param queue Priority blocking queue to obtain an element from.
 * @return The size of the queue.
 */
int PBQueue_Get_Size(PBQueue *queue);