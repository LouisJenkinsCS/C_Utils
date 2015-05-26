#include <pthread.h>
#include <stdlib.h>
#include "PBQueue.h"

/* Static functions */

static void Add_As_Head(PBQueue *queue, PBQ_Node *node){
	task->next = tp->queue->head;
	tp->queue->head = task;
}

static void Add_As_Tail(PBQueue *queue, PBQ_Node *node){
	tp->queue->tail->next = tp->queue->tail = task;
	task->next = NULL;
}

static void Add_As_Only(PBQueue *queue, PBQ_Node *node){
	task->next = NULL;
	tp->queue->head = tp->queue->tail = task;
}

static void Add_After(PBQueue *queue, PBQ_Node *node, PBQ_Node *node){
	task->next = previous_task->next;
	previous_task->next = task;
}

static void Add_Task_Sorted(Task *task){
	LOCK(tp->queue->adding_task);
	if(tp->queue->size == 0) Add_Task_As_Only(task); 
	else if(tp->queue->size == 1){
		if(task->priority > tp->queue->head->priority) Add_Task_As_Head(task);
		else Add_Task_As_Tail(task);
	} else if (task->priority == LOWEST_PRIORITY) Add_Task_As_Tail(task);
	else {
		Task *task_to_compare = NULL;
		// To avoid adding a doubly linked list, I keep track of the previous task.
		Task *previous_task = NULL;
		for(previous_task = task_to_compare = TP->queue->head; task_to_compare; previous_task = task_to_compare, task_to_compare = task_to_compare->next){
			if(task->priority > task_to_compare->priority) Add_Task_After(task, previous_task);
			else if (!task->next) Add_Task_As_Tail(task);
		}
	}
	tp->queue->size++;
	UNLOCK(tp->queue->adding_task);
}

static void Add_Item(PBQueue *queue, void *item){
	PBQ_Node *node = malloc(sizeof(PBQ_Node));
	node->item = item;
	if(queue->size == 0) Add_As_Only(queue, node);
	else if(queue->size == 1){
		// Checks to see if the item is of greater priority than the head of the queue.
		if(queue->comparator(void *item, queue->head->item) > 0) Add_Task_As_Head(node);
		else Add_Task_As_Tail(node);
	} else if(queue->comparator(void *item, queue->head->item) > 0) Add_Task_As_Head(node);
	else {
		PBQ_Node *current_node = PBQ_Node *previous_node = NULL;
		// START HERE!
	}
}

static void Take_Item(PBQueue *queue);

/* End static functions */

/// Returns an initialized bounded queue of max size max_elements.
PBQueue *PBQueue_Create_Bounded(size_t max_elements, compare_elements comparator){
	PBQueue *queue = malloc(sizeof(PBQueue));
	queue->size = 0;
	queue->max_size = max_elements;
	queue->head = queue->tail = NULL;
	queue->adding_or_removing_elements = malloc(sizeof(pthread_mutex_t));
	queue->is_not_full = malloc(sizeof(pthread_cond_t));
	queue->is_not_empty = malloc(sizeof(pthread_cond_t));
	pthread_cond_init(queue->is_not_full, NULL);
	pthread_cond_init(queue->is_not_empty, NULL);
	pthread_mutex_init(queue->adding_or_removing_elements, NULL);
	queue->type = PBQ_BOUNDED;
	queue->comparator = comparator;
	return queue;
}

/// Returns an initialized unbounded queue.
PBQueue *PBQueue_Create_Unbounded(compare_elements comparator){
	PBQueue *queue = malloc(sizeof(PBQueue));
	/// max_size is set to 0 here because it's irrelevant and not used for something unbounded.
	queue->size = queue->max_size = 0;
	queue->head = queue->tail = NULL;
	queue->adding_or_removing_elements = malloc(sizeof(pthread_mutex_t));
	queue->is_not_full = malloc(sizeof(pthread_cond_t));
	queue->is_not_empty = malloc(sizeof(pthread_cond_t));
	pthread_cond_init(queue->is_not_full, NULL);
	pthread_cond_init(queue->is_not_empty, NULL);
	pthread_mutex_init(queue->adding_or_removing_elements, NULL);
	queue->type = PBQ_UNBOUNDED;
	queue->comparator = comparator;
	return queue;
}

/// Blocks until another element can be inserted.
int PBQueue_Enqueue(PBQueue *queue, void *item){
	/// Does not take nulls.
	if(!item) return 0;
	pthread_mutex_lock(queue->adding_or_removing_elements);
	if(queue->type == PBQ_BOUNDED){
		while(queue->size == max_size) pthread_cond_wait(queue->is_not_full, queue->adding_or_removing_elements);
	}
	Add_Item(queue, item);
	pthread_cond_signal(queue->is_not_empty);
	pthread_mutex_unlock(queue->adding_or_removing_elements);
	return 1;
}

/// Blocks until available.
void *PBQueue_Dequeue(PBQueue *queue){
	pthread_mutex_lock(queue->adding_or_removing_elements);
	while(queue->size == 0) pthread_cond_wait(queue->is_not_empty, queue->adding_or_removing_elements);
	void *item = Take_Item(queue);
	pthread_mutex_unlock(pthread->adding_or_removing_elements);
	return item;
}

/// Tells if queue is empty.
int PBQueue_Is_Empty(PBQueue *queue){
	return queue->size == 0;
}

/// Tells if the queue is full.
int PBQueue_Is_Full(PBQueue *queue){
	return queue->size == queue->max_size;
}

/// Tells the current queue size.
int PBQueue_Get_Size(PBQueue *queue){
	return queue->size;
}