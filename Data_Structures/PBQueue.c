#include <PBQueue.h>

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Data_Structures/Logs/PBQueue.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

/* Static functions */

static void Add_As_Head(DS_PBQueue_t *queue, DS_Node_t *node){
	node->_single.next = queue->head;
	queue->head = node;
	atomic_fetch_add(&queue->size, 1);
}

static void Add_As_Tail(DS_PBQueue_t *queue, DS_Node_t *node){
	queue->tail->_single.next = node;
	queue->tail = node;
	node->_single.next = NULL;
	atomic_fetch_add(&queue->size, 1);
}

static void Add_As_Only(DS_PBQueue_t *queue, DS_Node_t *node){
	node->_single.next = NULL;
	queue->head = queue->tail = node;
	atomic_fetch_add(&queue->size, 1);
}

static void Add_After(DS_PBQueue_t *queue, DS_Node_t *this_node, DS_Node_t *previous_node){
	this_node->_single.next = previous_node->_single.next;
	previous_node->_single.next = this_node;
	atomic_fetch_add(&queue->size, 1);
}

static bool Add_Item(DS_PBQueue_t *queue, void *item){
	DS_Node_t *node = malloc(sizeof(DS_Node_t));
	if(!node){
		return false;
	}
	node->item = item;
	if(!queue->compare){
		Add_As_Tail(queue, node);
		return true;
	}
	if(queue->size == 0) Add_As_Only(queue, node);
	else if(queue->size == 1){
		// Checks to see if the item is of greater priority than the head of the queue.
		if(queue->compare(item, queue->head->item) > 0) Add_As_Head(queue, node);
		else Add_As_Tail(queue, node);
	} else if(queue->compare(item, queue->head->item) > 0) Add_As_Head(queue, node);
	else if(queue->compare(queue->tail->item, item) >= 0) Add_As_Tail(queue, node);
	else {
		DS_Node_t *current_node = NULL;
		DS_Node_t *previous_node = NULL;
		// START HERE!
		for(previous_node = current_node = queue->head; current_node; previous_node = current_node, current_node = current_node->_single.next){
			if(queue->compare(item, current_node->item) > 0){
				Add_After(queue, node, previous_node); 
				return true; 
			}
			else if(!current_node->_single.next){
				Add_As_Tail(queue, node);
				return true;
			}
		}
	}
	return true;
}

static void *Take_Item(DS_PBQueue_t *queue){
	DS_Node_t *node = queue->head;
	if(!node) return NULL;
	void *item = queue->head->item;
	queue->head = queue->head->_single.next;
	free(node);
	atomic_fetch_sub(&queue->size, 1);
	return item;
}

/* End static functions */

/// Returns an initialized bounded queue of max size max_elements.
DS_PBQueue_t *DS_PBQueue_create(size_t max_elements, DS_comparator_cb compare){
	bool mutex_init = false, not_full_init = false, not_empty_init = false;
	DS_PBQueue_t *queue = calloc(1, sizeof(DS_PBQueue_t));
	if(!queue){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	/// max_elements of 0 means unbounded.
	queue->max_size = max_elements;
	queue->head = queue->tail = NULL;
	queue->manipulating_queue = malloc(sizeof(pthread_mutex_t));
	if(!queue->manipulating_queue){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	queue->not_full = malloc(sizeof(pthread_cond_t));
	if(!queue->not_full){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	queue->not_empty = malloc(sizeof(pthread_cond_t));
	if(!queue->not_empty){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		goto error;
	}
	bool init_error = pthread_cond_init(queue->not_full, NULL);
	if(init_error){
		MU_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(errno));
		goto error;
	}
	not_full_init = true;
	init_error = pthread_cond_init(queue->not_empty, NULL);
	if(init_error){
		MU_LOG_ERROR(logger, "pthread_cond_init: '%s'", strerror(errno));
		goto error;
	}
	not_empty_init = true;
	init_error = pthread_mutex_init(queue->manipulating_queue, NULL);
	if(init_error){
		MU_LOG_ERROR(logger, "pthread_mutex_init: '%s'", strerror(errno));
		goto error;
	}
	mutex_init = true;
	queue->compare = compare;
	queue->shutting_down = ATOMIC_VAR_INIT(false);
	queue->threads_waiting = ATOMIC_VAR_INIT(0);
	queue->size = ATOMIC_VAR_INIT(0);
	return queue;

	error:
		if(queue){
			if(queue->not_full){
				if(not_full_init){
					pthread_cond_destroy(queue->not_full);
				}
				free(queue->not_full);
			}
			if(queue->not_empty){
				if(not_empty_init){
					pthread_cond_destroy(queue->not_empty);
				}
				free(queue->not_empty);
			}
			if(queue->manipulating_queue){
				if(mutex_init){
					pthread_mutex_destroy(queue->manipulating_queue);
				}
				free(queue->manipulating_queue);
			}
			free(queue);
		}
		return NULL;
}

/// Blocks until either another element can be inserted or the time ellapses.
bool DS_PBQueue_enqueue(DS_PBQueue_t *queue, void *item, long long int timeout){
	MU_ARG_CHECK(logger, false, queue);
	atomic_fetch_add(&queue->threads_waiting, 1);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;
	pthread_mutex_lock(queue->manipulating_queue);
	if(queue->max_size){
		while(atomic_load(&queue->shutting_down) == false && atomic_load(&queue->size) == queue->max_size){
			int errcode;
			if(timeout < 0){
				errcode = pthread_cond_wait(queue->not_full, queue->manipulating_queue);
			} else {
				errcode = pthread_cond_timedwait(queue->not_full, queue->manipulating_queue, &ts);
			}
			if(errcode){
				if(errcode != ETIMEDOUT){
					MU_LOG_ERROR(logger, "%s: '%s'", timeout < 0 ? "pthread_cond_wait" : "pthread_cond_timedwait", strerror(errno));
				}
				pthread_mutex_unlock(queue->manipulating_queue);
				atomic_fetch_sub(&queue->threads_waiting, 1);
				return false;
			}
		}
	}
	if(atomic_load(&queue->shutting_down) == true){
		pthread_mutex_unlock(queue->manipulating_queue);
		atomic_fetch_sub(&queue->threads_waiting, 1);
		return false;
	}
	Add_Item(queue, item);
	pthread_cond_signal(queue->not_empty);
	pthread_mutex_unlock(queue->manipulating_queue);
	atomic_fetch_sub(&queue->threads_waiting, 1);
	return 1;
}

/// Blocks until a new element is available or the amount of the time ellapses.
void *DS_PBQueue_dequeue(DS_PBQueue_t *queue, long long int timeout){
	MU_ARG_CHECK(logger, NULL, queue);
	atomic_fetch_add(&queue->threads_waiting, 1);
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout;
	pthread_mutex_lock(queue->manipulating_queue);
	while(atomic_load(&queue->shutting_down) == false && atomic_load(&queue->size) == 0){
		int errcode;
		if(timeout < 0){
			errcode = pthread_cond_wait(queue->not_empty, queue->manipulating_queue);
		} else {
			errcode = pthread_cond_timedwait(queue->not_empty, queue->manipulating_queue, &ts);
		}
		if(errcode){
			if(errcode != ETIMEDOUT){
				MU_LOG_ERROR(logger, "%s: '%s'", timeout < 0 ? "pthread_cond_wait" : "pthread_cond_timedwait", strerror(errno));
			}
			pthread_mutex_unlock(queue->manipulating_queue);
			atomic_fetch_sub(&queue->threads_waiting, 1);
			return false;
		}
	}
	void *item = Take_Item(queue);
	pthread_cond_signal(queue->not_full);
	pthread_mutex_unlock(queue->manipulating_queue);
	atomic_fetch_sub(&queue->threads_waiting, 1);
	return item;
}

/// Clear the queue, and optionally execute a callback on every item currently in the queue. I.E allows you to delete them.
bool DS_PBQueue_clear(DS_PBQueue_t *queue, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, queue);
	atomic_fetch_add(&queue->threads_waiting, 1);
	pthread_mutex_lock(queue->manipulating_queue);
	DS_Node_t *current_node = NULL;
	for(current_node = queue->head; current_node; current_node = queue->head){
		if(del) del(current_node->item);
		queue->head = current_node->_single.next;
		free(current_node);
		atomic_fetch_sub(&queue->size, 1);
	}
	queue->tail = NULL;
	pthread_mutex_unlock(queue->manipulating_queue);
	atomic_fetch_sub(&queue->threads_waiting, 1);
	return true;
}

/// Clears the queue then destroys the queue. Will execute a callback on every item in the queue if not null.
bool PBQueue_Destroy(DS_PBQueue_t *queue, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, queue);
	DS_PBQueue_clear(queue, del);
	pthread_mutex_lock(queue->manipulating_queue);
	atomic_store(&queue->shutting_down, true);
	pthread_cond_broadcast(queue->not_full);
	pthread_cond_broadcast(queue->not_empty);
	pthread_mutex_unlock(queue->manipulating_queue);
	while(atomic_load(&queue->threads_waiting) > 0){
		pthread_yield();
	}
	pthread_cond_destroy(queue->not_full);
	pthread_cond_destroy(queue->not_empty);
	pthread_mutex_destroy(queue->manipulating_queue);
	free(queue->not_full);
	free(queue->not_empty);
	free(queue->manipulating_queue);
	free(queue);
	return true;
}