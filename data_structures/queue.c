#include "queue.h"

#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#include "../memory/hazard.h"
#include "../io/logger.h"
#include "../misc/alloc_check.h"
#include "../misc/argument_check.h"

struct c_utils_queue {
	struct c_utils_node *head;
	struct c_utils_node *tail;
	volatile size_t size;
};

static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./data_structures/logs/queue.log", "w", C_UTILS_LOG_LEVEL_ALL);

struct c_utils_queue *c_utils_queue_create(void) {
 	struct c_utils_queue *queue;
	C_UTILS_ON_BAD_CALLOC(queue, logger, sizeof(*queue))
		goto err;

	// Our dummy node, the queue will always contain one element.
	struct c_utils_node *node;
	C_UTILS_ON_BAD_CALLOC(node, logger, sizeof(*node))
		goto err_node;

	queue->head = queue->tail = node;
	
	return queue;

	err_node:
		free(queue);
	err:
		return NULL;
}

bool c_utils_queue_enqueue(struct c_utils_queue *queue, void *data) {
	C_UTILS_ARG_CHECK(logger, false, queue);

	struct c_utils_node *node;
	C_UTILS_ON_BAD_CALLOC(node, logger, sizeof(*node))
		return false;
	node->item = data;

	struct c_utils_node *tail;
	struct c_utils_node *next;
	while (true) {
		tail = queue->tail;
		c_utils_hazard_acquire(0, tail);
		// Sanity check.
		if (tail != queue->tail) {
			pthread_yield();
			continue;
		}
		
		next = tail->_single.next;
		// Sanity check.
		if (tail != queue->tail) {
			pthread_yield();
			continue;
		}
		
		// Note that there should be no next node, as this is the tail.
		if (next != NULL) {
			// If there is one, we fix it here.
			__sync_bool_compare_and_swap(&queue->tail, tail, next);
			pthread_yield();
			continue;
		}

		// Append the node to the tail.
		if (__sync_bool_compare_and_swap(&tail->_single.next, NULL, node)) 
			break;

		pthread_yield();
	}
	// In case another thread had already CAS the tail forward, we conditionally do so here.
	__sync_bool_compare_and_swap(&queue->tail, tail, node);
	c_utils_hazard_release(tail, false);
	
	return true;
}

void *c_utils_queue_dequeue(struct c_utils_queue *queue) {
	C_UTILS_ARG_CHECK(logger, NULL, queue);
	
	struct c_utils_node *head, *tail, *next;
	void *item;
	while (true) {
		head = queue->head;
		c_utils_hazard_acquire(0, head);
		// Sanity check.
		if (head != queue->head) {
			pthread_yield();
			continue;
		}
		
		tail = queue->tail;
		next = head->_single.next;
		c_utils_hazard_acquire(1, next);
		// Sanity check.
		if (head != queue->head) {
			pthread_yield();
			continue;
		}
		
		// Is Empty.
		if (next == NULL) {
			c_utils_hazard_release_all(false);
			return NULL;
		}
		
		// If Head and Tail are the same, yet is not empty, then it is outdated.
		if (head == tail) {
			//Fix it!
			__sync_bool_compare_and_swap(&queue->tail, tail, next);
			pthread_yield();
			continue;
		}

		// Note that it takes the next node's item, not the current node, which serves as the dummy.
		item = next->item;
		if (__sync_bool_compare_and_swap(&queue->head, head, next)) 
			break;

		pthread_yield();
	}
	// We make sure to retire the head (or old head) popped from the queue, but not the next node (or new head).
	c_utils_hazard_release(head, true);
	c_utils_hazard_release(next, false);
	
	return item;
}

bool c_utils_queue_destroy(struct c_utils_queue *queue, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, false, queue);
	
	c_utils_hazard_release_all(false);
	
	struct c_utils_node *prev_node = NULL, *node;
	for (node = queue->head; node; node = node->_single.next) {
		free(prev_node);
		if (del) 
			del(node->item);

		prev_node = node;
	}
	free(prev_node);
	free(queue);
	
	return true;
}
