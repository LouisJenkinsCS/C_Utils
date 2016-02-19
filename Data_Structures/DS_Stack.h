#ifndef DS_STACK_H
#define DS_STACK_H

#include <DS_Helpers.h>
#include <stdint.h>
#include <stdatomic.h>

<<<<<<< HEAD
=======
/*
	A lock-free implementation of a stack. It is guaranteed to be wait-free, and supports basic
	stack manipulations.
*/

>>>>>>> development
typedef struct {
	DS_Node_t *head;
	volatile size_t size;
} DS_Stack_t;

DS_Stack_t *DS_Stack_create(void);

<<<<<<< HEAD
bool DS_Stack_push(DS_Stack_t *stack, void *item);

void *DS_Stack_pop(DS_Stack_t *stack);

=======
/*
	Push the item down on the stack. Returns true unless there is an
	allocation failure.
*/
bool DS_Stack_push(DS_Stack_t *stack, void *item);

/*
	Returns the item on top of the stack, returning null if either
	there is an allocation failure or if the stack is empty.
*/
void *DS_Stack_pop(DS_Stack_t *stack);

/*
	Destroys the stack, as well as all nodes, and optionally each node's item
	if the callback del is declared.
*/
>>>>>>> development
bool DS_Stack_destroy(DS_Stack_t *stack, DS_delete_cb del);

#endif /* endif DS_STACK_H */