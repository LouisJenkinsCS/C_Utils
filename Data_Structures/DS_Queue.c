#include <DS_Queue.h>

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Data_Structures/Logs/DS_Queue.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

static DS_Atomic_Node_t *DS_Atomic_Node_create(void *item){
	DS_Atomic_Node_t *node = calloc(1, sizeof(DS_Atomic_Node_t));
	if(!node){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	node->ptr = calloc(1, sizeof(DS_Atomic_Pointer_t));
	if(!node->ptr){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		free(node);
		return NULL;
	}
	node->ptr->next = ATOMIC_VAR_INIT(NULL);
	node->ptr->id = ATOMIC_VAR_INIT(0);
	return node;
}

DS_Queue_t *DS_Queue_create(void){
	DS_Queue_t *queue = calloc(1, sizeof(DS_Queue_t));
	if(!queue){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}
	DS_Atomic_Node_t *node = DS_Atomic_Node_create(NULL);
	queue->head = ATOMIC_VAR_INIT(NULL);
	queue->tail = ATOMIC_VAR_INIT(NULL);
	atomic_store(queue->head, *node);
	queue->tail = queue->head;
	return queue;

	error:
		return NULL;
}

bool DS_Queue_enqueue(DS_Queue_t *queue, void *data){
	MU_ARG_CHECK(logger, false, queue);
	DS_Atomic_Node_t *node = DS_Atomic_Node_create(data);
	if(!node){
		MU_LOG_ERROR(logger, "DS_Atomic_Node_create: 'Was unable to create an atomic node!'");
		return false;
	}
	DS_Atomic_Node_t tail;
	bool enqueued = false;
	do {
		/// Get tail from queue, atomically.
		tail = queue->head;
		/// Increment the id to prevent aba problem on change
		atomic_fetch_add(&tail.ptr->id, 1);
		/// If the tail is the last node, set node equal to tail.
		enqueued = atomic_compare_exchange_weak(tail.ptr->next, NULL, *node);
		/// If somehow, the tail of the queue isn't the last, then something went wrong
		if(!enqueued){
			/// So we set the new tail as the next one as a way of self-correcting.
			atomic_compare_exchange_weak(queue->tail, &tail, *tail.ptr->next);
		}
	} while(!enqueued);
	atomic_compare_exchange_strong(queue->tail, &tail, *node);
	return true;
}

void *DS_Queue_dequeue(DS_Queue_t *queue, void *data){
	DS_Atomic_Node_t head;
	do {
		head = atomic_load(queue->head);
	} while()
}

void *DS_Queue_dequeue(DS_Queue_t *queue);

size_t DS_Queue_size(DS_Queue_t *queue);

bool DS_Queue_destroy(DS_Queue_t *queue);
