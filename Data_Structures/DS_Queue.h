#ifndef DS_QUEUE_H
#define DS_QUEUE_H

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

struct DS_Atomic_Pointer_t {
	/// Next atomic pointer.
	_Atomic DS_Atomic_Node_t *next;
	/// Atomic identifier to help address ABA problem.
	_Atomic uint64_t id;
};

typedef struct {
	_Atomic DS_Atomic_Node_t *head;
	_Atomic DS_Atomic_Node_t *tail;
	_Atomic size_t size;
} DS_Queue_t;

DS_Queue_t *DS_Queue_create(void);

bool DS_Queue_enqueue(DS_Queue_t *queue, void *data);

void *DS_Queue_dequeue(DS_Queue_t *queue);

size_t DS_Queue_size(DS_Queue_t *queue);

bool DS_Queue_destroy(DS_Queue_t *queue);

#endif /* endif DS_QUEUE_H */