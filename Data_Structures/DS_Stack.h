#ifndef DS_STACK_H
#define DS_STACK_H

#include <DS_Helpers.h>
#include <stdint.h>
#include <stdatomic.h>

typedef struct DS_Atomic_Pointer_t DS_Atomic_Pointer_t;

typedef struct DS_Atomic_Node_t {
	/// Pointer to next atomic node.
	struct DS_Atomic_Pointer_t *ptr;
	/// Data in this node.
	void *data;
} DS_Atomic_Node_t;

/// Aligned to be of size 8, A.K.A a Double Word.
struct DS_Atomic_Pointer_t {
	/// Next atomic pointer.
	DS_Atomic_Node_t *next;
	/// Atomic identifier to help address ABA problem.
	unsigned int counter;
};

typedef struct {
	DS_Atomic_Node_t *head;
	size_t size;
} DS_Stack_t;

DS_Stack_t *DS_Stack_create(void);

bool DS_Stack_push(DS_Stack_t *stack, void *item);

void *DS_Stack_pop(DS_Stack_t *stack);

size_t DS_Stack_size(DS_Stack_t *stack);

size_t DS_Stack_destroy(DS_Stack_t *stack);

#endif /* endif DS_STACK_H */