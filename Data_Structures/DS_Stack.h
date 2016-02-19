#ifndef DS_STACK_H
#define DS_STACK_H

#include <DS_Helpers.h>
#include <stdint.h>
#include <stdatomic.h>

typedef struct {
	DS_Node_t *head;
	volatile size_t size;
} DS_Stack_t;

DS_Stack_t *DS_Stack_create(void);

bool DS_Stack_push(DS_Stack_t *stack, void *item);

void *DS_Stack_pop(DS_Stack_t *stack);

bool DS_Stack_destroy(DS_Stack_t *stack, DS_delete_cb del);

#endif /* endif DS_STACK_H */