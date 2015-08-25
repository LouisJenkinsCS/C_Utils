#include <DS_Stack.h>

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Data_Structures/Logs/DS_Stack.log", "w", MU_ALL);
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
	return node;
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
	MU_ARG_CHECK(logger, false, stack);
	DS_Atomic_Node_t *node = DS_Atomic_Node_create(item);
	DS_Atomic_Node_t old_head, new_head;
	bool pushed = false;
	do {
		__sync_val_compare_and_swap(stack->head, NULL, *node);
	} while(!atomic_compare_exchange_weak(stack->head, &old_head, new_head));
}

void *DS_Stack_pop(DS_Stack_t *stack);

size_t DS_Stack_size(DS_Stack_t *stack);

size_t DS_Stack_destroy(DS_Stack_t *stack);