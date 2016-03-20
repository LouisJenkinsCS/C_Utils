#include "stack.h"

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
	struct c_utils_stack_conf conf;
};



static void push(struct c_utils_stack *stack, struct c_utils_node *node);

static void lock_free_push(struct c_utils_stack *stack, struct c_utils_node *node);

static void *pop(struct c_utils_stack *stack);

static void *lock_free_pop(struct c_utils_stack *stack);



struct c_utils_stack *c_utils_stack_create(void) {
	struct c_utils_stack_conf conf;
	return c_utils_stack_create_conf(&conf);
}

struct c_utils_stack *c_utils_stack_create_conf(struct c_utils_stack_conf *conf) {
	if(!conf)
		return NULL;

	struct c_utils_stack *stack;
	C_UTILS_ON_BAD_CALLOC(stack, conf->logger, sizeof(*stack))
		return NULL;

	stack->conf = *conf;
	return stack;
}

bool c_utils_stack_push(struct c_utils_stack *stack, void *item) {
	if(!stack)
		return false;

	if(!item) {
		C_UTILS_LOG_ERROR(stack->conf.logger, "This stack does not support NULL items!");
		return false;
	}

	struct c_utils_node *node;
	C_UTILS_ON_BAD_CALLOC(node, stack->conf.logger, sizeof(*node))
		return false;
	node->item = item;

	if(stack->conf.lock_free)
		lock_free_push(stack, node);
	else
		push(stack, node);

	return true;
}

void *c_utils_stack_pop(struct c_utils_stack *stack) {
	if(!stack)
		return NULL;

	if(stack->conf.lock_free)
		return lock_free_pop(stack);
	else
		return pop(stack);
}

bool c_utils_stack_destroy(struct c_utils_stack *stack) {
	if(!stack)
		return NULL;
	
	if(stack->conf.lock_free)
		c_utils_hazard_release_all(false);
	
	struct c_utils_node *prev_node = NULL;
	for (struct c_utils_node *node = stack->head; node; node = node->next) {
		free(prev_node);
		if (stack->conf.del)
			stack->conf.del(node->item);
		
		prev_node = node;
	}
	free(prev_node);
	free(stack);
	
	return true;
}



static void push(struct c_utils_stack *stack, struct c_utils_node *node) {
	node->next = stack->head;
	stack->head = node;
	stack->size++;
}

static void lock_free_push(struct c_utils_stack *stack, struct c_utils_node *node) {
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
		
		node->next = head;
		if (__sync_bool_compare_and_swap(&stack->head, head, node))
			break;
		
		pthread_yield();
	}
	if (head)
		c_utils_hazard_release(head, false);
	
	__sync_fetch_and_add(&stack->size, 1);
}

static void *pop(struct c_utils_stack *stack) {
	if(!stack->head)
		return NULL;

	struct c_utils_node *head = stack->head;
	void *item = head->item;

	stack->head = head->next;
	free(head);
	stack->size--;

	return item;
}

static void *lock_free_pop(struct c_utils_stack *stack) {
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

		if (__sync_bool_compare_and_swap(&stack->head, head, stack->head->next))
			break;

		pthread_yield();
	}
	void *data = head->item;

	c_utils_hazard_release(head, true);
	__sync_fetch_and_sub(&stack->size, 1);

	return data;
}
