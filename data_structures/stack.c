#include <DS_Stack.h>

#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#include "../memory/hazard.h"
#include "../io/logger.h"
#include "../misc/alloc_check.h"
#include "../misc/argument_check.h"

struct c_utils_stack {
	struct c_utils_node *head;
	volatile size_t size;
};

static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./data_structures/logs/stack.log", "w", C_UTILS_LOG_LEVEL_ALL);

struct c_utils_stack *DS_Stack_create(void) {
	struct c_utils_stack *stack;
	C_UTILS_ON_BAD_CALLOC(stack, logger, sizeof(*stack))
		return NULL;

	return stack;
}

bool DS_Stack_push(struct c_utils_stack *stack, void *item) {
	C_UTILS_ARG_CHECK(logger, false, stack, item);

	struct c_utils_node *node;
	C_UTILS_ON_BAD_CALLOC(node, logger, sizeof(*node))
		return false;

	struct c_utils_node *head;
	while (true) {
		head = stack->head;
		if (head) 
			c_utils_hazard_acquire(0, head);

		// Ensures head isn't freed before it was tagged by hazard pointer.
		if (head != stack->head) {
			pthread_yield();
			continue;
		}
		
		node->_single.next = head;
		if (__sync_bool_compare_and_swap(&stack->head, head, node)) 
			break;
		
		pthread_yield();
	}
	if (head) 
		c_utils_hazard_release(head, false);
	
	__sync_fetch_and_add(&stack->size, 1);
	
	return true;
}

void *DS_Stack_pop(struct c_utils_stack *stack) {
	C_UTILS_ARG_CHECK(logger, NULL, stack);
	
	struct c_utils_node *head;
	while (true) {
		head = stack->head;
		if (!head) 
			return NULL;
		
		c_utils_hazard_acquire(0, head);
		if (head != stack->head) {
			pthread_yield();
			continue;
		}

		if (__sync_bool_compare_and_swap(&stack->head, head, stack->head->_single.next)) 
			break;
		
		pthread_yield();
	}
	void *data = head->item;
	
	c_utils_hazard_release(head, true);
	__sync_fetch_and_sub(&stack->size, 1);
	
	return data;
}

bool DS_Stack_destroy(struct c_utils_stack *stack, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, false, stack);
	
	c_utils_hazard_release_all(false);
	
	struct c_utils_node *prev_node = NULL;
	for (struct c_utils_node *node = stack->head; node; node = node->_single.next) {
		free(prev_node);
		if (del) 
			del(node->item);
		
		prev_node = node;
	}
	free(prev_node);
	free(stack);
	
	return true;
}