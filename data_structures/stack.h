#ifndef C_UTILS_STACK_H
#define C_UTILS_STACK_H

#include <stdbool.h>

#include "helpers.h"

/*
	A mostly lock-free implementation of a stack. It is guaranteed to be wait-free, and supports basic
	stack manipulations. 

	It is "mostly" lock-free as the lock-free memory allocator is not currently implemented, hence
	malloc/calloc use its own internal lock for it's heap. Otherwise, it is lock-free.
*/
struct c_utils_stack;

/*
	No typedef for stack, as it is taken in POSIX standard.
*/

#ifdef NO_C_UTILS_PREFIX

/*
	Functions
*/
#define stack_create(...) c_utils_stack_create(__VA_ARGS__)
#define stack_push(...) c_utils_stack_push(__VA_ARGS__)
#define stack_pop(...) c_utils_stack_pop(__VA_ARGS__)
#define stack_destroy(...) c_utils_stack_destroy(__VA_ARGS__)
#endif




struct c_utils_stack *c_utils_stack_create(void);

/*
	Push the item down on the stack. Returns true unless there is an
	allocation failure.
*/
bool c_utils_stack_push(struct c_utils_stack *stack, void *item);

/*
	Returns the item on top of the stack, returning null if either
	there is an allocation failure or if the stack is empty.
*/
void *c_utils_stack_pop(struct c_utils_stack *stack);

/*
	Destroys the stack, as well as all nodes, and optionally each node's item
	if the callback del is declared.
*/
bool c_utils_stack_destroy(struct c_utils_stack *stack, c_utils_delete_cb del);

#endif /* endif C_UTILS_STACK_H */