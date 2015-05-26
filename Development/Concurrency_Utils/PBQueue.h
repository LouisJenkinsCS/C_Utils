#include <pthread.h>

typedef struct PBQueue PBQueue;

typedef struct PBQ_Node PBQ_Node;

typedef enum PBQ_Type PBQ_Type;

typedef int (*compare_elements)(void *item_one, void *item_two);

struct PBQ_Node{
	/// Pointer to the next node.
	PBQ_Node *next;
	/// The user's item added to the queue.
	void *item;
};

enum PBQ_Type{
	/// The queue's size if infinite (or until out of memory).
	PBQ_UNBOUNDED,
	/// The queue's size is based on 
	PBQ_BOUNDED
}

struct PBQueue {
	/// A pointer to the head node.
	PBQ_Node *head;
	/// A pointer to the tail node.
	PBQ_Node *tail;
	/// To compare elements to determine priority.
	compare_elements comparator;
	/// Determines whether queue is bounded or unbounded.
	PBQ_Type type;
	/// Size of the current queue, meaning amount of elements currently in the queue.
	volatile size_t size;
	/// The maximum size of the queue if it is bounded.
	size_t max_size;
	/// A new element may be added to the PBQueue.
	pthread_cond_t *is_not_full;
	/// A element has been added and may be removed from the PBQueue.
	pthread_cond_t *is_not_empty;
	/// The lock used to add or remove an element to/from the queue (respectively).
	pthread_mutex_t *adding_or_removing_elements;
};

/// Returns an initialized bounded queue of max size max_elements.
PBQueue *PBQueue_Create_Bounded(size_t max_elements, compare_elements comparator);

/// Returns an initialized unbounded queue.
PBQueue *PBQueue_Create_Unbounded(compare_elements comparator);

/// Blocks until another element can be inserted.
int PBQueue_Enqueue(PBQueue *queue, void *item);

/// Blocks until available.
void *PBQueue_Dequeue(PBQueue *queue);

/// Tells if queue is empty.
int PBQueue_Is_Empty(PBQueue *queue);

/// Tells if the queue is full.
int PBQueue_Is_Full(PBQueue *queue);

/// Tells the current queue size.
int PBQueue_Get_Size(PBQueue *queue);