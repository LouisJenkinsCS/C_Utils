#include <DS_Queue.h>

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Data_Structures/Logs/DS_Queue.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

DS_Queue_t *DS_Queue_create(void){
	DS_Queue_t *queue = calloc(1, sizeof(DS_Queue_t));
	if(!queue){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	// Our dummy node, the queue will always contain one element.
	DS_Node_t *node = DS_Node_create(NULL, logger);
	if(!node){
		MU_LOG_ERROR(logger, "DS_Node_create: 'Was unable to create a node!'");
		goto error;
	}
	return queue;

	error:
		if(queue){
			free(queue);
		}
		return NULL;
}

bool DS_Queue_enqueue(DS_Queue_t *queue, void *data){
	MU_ARG_CHECK(logger, false, queue);
	DS_Node_t *node = DS_Node_create(data, logger);
	if(!node){
		MU_LOG_ERROR(logger, "DS_Node_create: 'Was unable to create a node!'");
		return false;
	}
	DS_Node_t *tail;
	DS_Node_t *next;
	while(true){
		tail = queue->tail;
		MU_Hazard_Pointer_acquire(0, tail);
		// Sanity check.
		if(tail != queue->tail){
			pthread_yield();
			continue;
		}
		next = tail->_single.next;
		// Sanity check.
		if(tail != queue->tail){
			pthread_yield();
			continue;
		}
		// Note that there should be no next node, as this is the tail.
		if(next != NULL){
			// If there is one, we fix it here.
			__sync_bool_compare_and_swap(&(queue->tail), tail, next);
			pthread_yield();
			continue;
		}
		// Append the node to the tail.
		if(__sync_bool_compare_and_swap(&(tail->_single.next), NULL, node)) break;
		pthread_yield();
	}
	// In case another thread had already CAS the tail forward, we conditionally do so here.
	__sync_bool_compare_and_swap(&(queue->tail), tail, node);
	MU_Hazard_Pointer_release(tail, false);
	return true;
}

void *DS_Queue_dequeue(DS_Queue_t *queue){
	MU_ARG_CHECK(logger, NULL, queue);
	DS_Node_t *head, *tail, *next;
	void *item;
	while(true){
		head = queue->head;
		MU_Hazard_Pointer_acquire(0, head);
		// Sanity check.
		if(head != queue->head){
			pthread_yield();
			continue;
		}
		tail = queue->tail;
		next = head->_single.next;
		MU_Hazard_Pointer_acquire(1, next);
		// Sanity check.
		if(head != queue->head){
			pthread_yield();
			continue;
		}
		// Is Empty.
		if(next == NULL) return NULL;
		// If Head and Tail are the same, yet is not empty, then it is outdated.
		if(head == tail){
			//Fix it!
			__sync_bool_compare_and_swap(&(queue->tail), tail, next);
			pthread_yield();
			continue;
		}
		// Note that it takes the next node's item, not the current node, which serves as the dummy.
		item = next->item;
		if(__sync_bool_compare_and_swap(&(queue->head), head, next)) break;
		pthread_yield();
	}
	// We make sure to retire the head (or old head) popped from the queue, but not the next node (or new head).
	MU_Hazard_Pointer_release(head, true);
	MU_Hazard_Pointer_release(next, false);
	return item;
}

bool DS_Queue_destroy(DS_Queue_t *queue, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, queue);
	MU_Hazard_Pointer_release_all(false);
	DS_Node_t *prev_node = NULL, *node;
	for(node = queue->head; node; node = node->next){
		free(prev_node);
		if(del) del(node->item);
		prev_node = node;
	}
	free(prev_node);
	free(queue);
	return true;
}
