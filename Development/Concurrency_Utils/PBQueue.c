#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "PBQueue.h"

/* Static functions */

static void Add_As_Head(PBQueue *queue, PBQ_Node *node){
	node->next = queue->head;
	queue->head = node;
	queue->size++;
}

static void Add_As_Tail(PBQueue *queue, PBQ_Node *node){
	queue->tail->next = node;
	queue->tail = node;
	node->next = NULL;
	queue->size++;
}

static void Add_As_Only(PBQueue *queue, PBQ_Node *node){
	node->next = NULL;
	queue->head = queue->tail = node;
	queue->size++;
}

static void Add_After(PBQueue *queue, PBQ_Node *this_node, PBQ_Node *previous_node){
	this_node->next = previous_node->next;
	previous_node->next = this_node;
	queue->size++;
}

static void Add_Item(PBQueue *queue, void *item){
	PBQ_Node *node = malloc(sizeof(PBQ_Node));
	node->item = item;
	if(queue->size == 0) Add_As_Only(queue, node);
	else if(queue->size == 1){
		// Checks to see if the item is of greater priority than the head of the queue.
		if(queue->comparator(item, queue->head->item) > 0) Add_As_Head(queue, node);
		else Add_As_Tail(queue, node);
	} else if(queue->comparator(item, queue->head->item) > 0) Add_As_Head(queue, node);
	else {
		PBQ_Node *current_node = NULL;
		PBQ_Node *previous_node = NULL;
		// START HERE!
		for(previous_node = current_node = queue->head; current_node; previous_node = current_node, current_node = current_node->next){
			if(queue->comparator(item, current_node->item) > 0) { Add_After(queue, node, previous_node); return; }
			else if(!current_node->next) Add_As_Tail(queue, node);
		}
	}
}

static void *Take_Item(PBQueue *queue){
	PBQ_Node *node = queue->head;
	void *item = queue->head->item;
	queue->head = queue->head->next;
	free(node);
	queue->size--;
	return item;
}

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
		//printf("Queue Size: %d\n", queue->size);
		while(queue->size == queue->max_size) {
			pthread_cond_wait(queue->is_not_full, queue->adding_or_removing_elements);
		}
	}
	Add_Item(queue, item);
	pthread_cond_signal(queue->is_not_empty);
	pthread_mutex_unlock(queue->adding_or_removing_elements);
	return 1;
}

/// Blocks until either another element can be inserted or the time ellapses.
int PBQueue_Timed_Enqueue(PBQueue *queue, void *item, unsigned int seconds){
	if(!item) return 0;
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += seconds;
	pthread_mutex_lock(queue->adding_or_removing_elements);
	if(queue->type == PBQ_BOUNDED){
		while(queue->size == queue->max_size) pthread_cond_timedwait(queue->is_not_full, queue->adding_or_removing_elements, &timeout);
	}
	Add_Item(queue, item);
	pthread_cond_signal(queue->is_not_empty);
	pthread_mutex_unlock(queue->adding_or_removing_elements);
	return 1;
}

/// Blocks until available.
void *PBQueue_Dequeue(PBQueue *queue){
	pthread_mutex_lock(queue->adding_or_removing_elements);
	while(queue->size == 0) { 
		pthread_cond_wait(queue->is_not_empty, queue->adding_or_removing_elements);
	}
	void *item = Take_Item(queue);
	pthread_cond_signal(queue->is_not_full);
	pthread_mutex_unlock(queue->adding_or_removing_elements);
	return item;
}

/// Blocks until a new element is available or the amount of the time ellapses.
void *PBQueue_Timed_Dequeue(PBQueue *queue, unsigned int seconds){
	pthread_mutex_lock(queue->adding_or_removing_elements);
	struct timespec timeout;
	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout.tv_sec += seconds;
	while(queue->size == 0) pthread_cond_timedwait(queue->is_not_empty, queue->adding_or_removing_elements, &timeout);
	void *item = Take_Item(queue);
	pthread_mutex_unlock(queue->adding_or_removing_elements);
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