#include <DS_Stack.h>
#include <MU_Hazard_Pointers.h>

static thread_local MU_Hazard_Pointer_t *hp = NULL;

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Data_Structures/Logs/DS_Stack.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}

static DS_Node_t *DS_Node_create(void *item){
	DS_Node_t *node = calloc(1, sizeof(*node);
	if(!node){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	node->item = item;
	return node;
}

DS_Stack_t *DS_Stack_create(void){
	DS_Stack_t *stack = calloc(1, sizeof(DS_Stack_t));
	if(!stack){
		MU_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	if(!hp){
		hp = MU_Hazard_Pointer_acquire();
	}
	return stack;
}

bool DS_Stack_push(DS_Stack_t *stack, void *item){
	MU_ARG_CHECK(logger, false, stack, item);
	DS_Node_t *node = DS_Node_create(item);
	DS_Node_t *head;
	while(true){
		head = stack->head;
		hp->owned[0] = head;
		// Ensures head isn't freed before it was tagged by hazard pointer.
		if(head != stack->head) continue;
		node->_single.next = head;
		if(__sync_bool_compare_and_swap(stack->head, head, node)) break;
	}
	MU_Hazard_Pointer_reset(hp);
	return true;
}

void *DS_Stack_pop(DS_Stack_t *stack){
	MU_ARG_CHECK(logger, NULL, stack);
	DS_Node_t *head;
	while(true){
		head = stack->head;
		if(!head) return NULL;
		hp->owned[0] = head;
		if(head != stack->head) continue;
		if(__sync_bool_compare_and_swap(stack->head, head, stack->head->_single.next)) break;
	}
	void *data = head->item;
	MU_Hazard_Pointer_retire(hp, head);
	return data;
}

size_t DS_Stack_size(DS_Stack_t *stack);

size_t DS_Stack_destroy(DS_Stack_t *stack);