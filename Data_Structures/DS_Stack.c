#include <DS_Stack.h>
#include <stdatomic.h>
#include <unistd.h>
#include <MU_Hazard_Pointers.h>

#define DS_STACK_BACK_OFF pthread_yield()

static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./Data_Structures/Logs/DS_Stack.log", "w", MU_ALL);

DS_Stack_t *DS_Stack_create(void) {
	DS_Stack_t *stack = calloc(1, sizeof(DS_Stack_t));
	if (!stack) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	return stack;
}

bool DS_Stack_push(DS_Stack_t *stack, void *item) {
	MU_ARG_CHECK(logger, false, stack, item);
	DS_Node_t *node = DS_Node_create(item, logger);
	if (!node) {
		C_UTILS_LOG_ERROR(logger, "DS_Node_create: 'Was unable to create a node!'");
		return false;
	}
	DS_Node_t *head;
	while (true) {
		head = stack->head;
		if (head) MU_Hazard_Pointer_acquire(0, head);
		// Ensures head isn't freed before it was tagged by hazard pointer.
		if (head != stack->head) {
			DS_STACK_BACK_OFF;
			continue;
		}
		node->_single.next = head;
		if (__sync_bool_compare_and_swap(&stack->head, head, node)) break;
		DS_STACK_BACK_OFF;
	}
	if (head) MU_Hazard_Pointer_release(head, false);
	__sync_fetch_and_add(&stack->size, 1);
	return true;
}

void *DS_Stack_pop(DS_Stack_t *stack) {
	MU_ARG_CHECK(logger, NULL, stack);
	DS_Node_t *head;
	while (true) {
		head = stack->head;
		if (!head) return NULL;
		MU_Hazard_Pointer_acquire(0, head);
		if (head != stack->head) {
			DS_STACK_BACK_OFF;
			continue;
		}
		if (__sync_bool_compare_and_swap(&stack->head, head, stack->head->_single.next)) break;
		DS_STACK_BACK_OFF;
	}
	void *data = head->item;
	MU_Hazard_Pointer_release(head, true);
	__sync_fetch_and_sub(&stack->size, 1);
	return data;
}

bool DS_Stack_destroy(DS_Stack_t *stack, DS_delete_cb del) {
	MU_ARG_CHECK(logger, false, stack);
	MU_Hazard_Pointer_release_all(false);
	DS_Node_t *prev_node = NULL;
	for (DS_Node_t *node = stack->head; node; node = node->_single.next) {
		free(prev_node);
		if (del) del(node->item);
		prev_node = node;
	}
	free(prev_node);
	free(stack);
	return true;
}