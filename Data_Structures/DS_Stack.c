#include <DS_Stack.h>
#include <stdatomic.h>
#include <unistd.h>
#include <MU_Hazard_Pointers.h>

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Data_Structures/Logs/DS_Stack.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

DS_Stack_t *DS_Stack_create(void){
	DS_Stack_t *stack = calloc(1, sizeof(DS_Stack_t));
	if(!stack){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	return stack;
}

bool DS_Stack_push(DS_Stack_t *stack, void *item){
	MU_ARG_CHECK(logger, false, stack, item);
	DS_Node_t *node = DS_Node_create(item, logger);
	if(!node){
		MU_LOG_ERROR(logger, "DS_Node_create: 'Was unable to create a node!'");
		return false;
	}
	DS_Node_t *head;
	while(true){
		head = stack->head;
		if(head) MU_Hazard_Pointer_acquire(0, head);
		// Ensures head isn't freed before it was tagged by hazard pointer.
		if(head != stack->head){
			pthread_yield();
			continue;
		}
		node->_single.next = head;
		if(__sync_bool_compare_and_swap(&stack->head, head, node)) break;
		pthread_yield();
	}
	if(head) MU_Hazard_Pointer_release(head, false);
	__sync_fetch_and_add(&stack->size, 1);
	return true;
}

void *DS_Stack_pop(DS_Stack_t *stack){
	MU_ARG_CHECK(logger, NULL, stack);
	DS_Node_t *head;
	while(true){
		head = stack->head;
		if(!head) return NULL;
		MU_Hazard_Pointer_acquire(0, head);
		if(head != stack->head){
			pthread_yield();
			continue;
		}
		if(__sync_bool_compare_and_swap(&stack->head, head, stack->head->_single.next)) break;
		pthread_yield();
	}
	void *data = head->item;
	MU_Hazard_Pointer_release(head, true);
	__sync_fetch_and_sub(&stack->size, 1);
	return data;
}

bool DS_Stack_destroy(DS_Stack_t *stack, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, stack);
	MU_Hazard_Pointer_release_all(false);
	DS_Node_t *prev_node = NULL;
	for(DS_Node_t *node = stack->head; node; node = node->_single.next){
		free(prev_node);
		if(del) del(node->item);
		prev_node = node;
	}
	free(prev_node);
	free(stack);
	return true;
}