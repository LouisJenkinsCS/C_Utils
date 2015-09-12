#ifndef DS_QUEUE_H
#define DS_QUEUE_H

#include <DS_Helpers.h>
#include <MU_Hazard_Pointers.h>
#include <stdint.h>
#include <stdatomic.h>

typedef struct {
	DS_Node_t *head;
	DS_Node_t *tail;
	volatile size_t size;
} DS_Queue_t;

DS_Queue_t *DS_Queue_create(void);

bool DS_Queue_enqueue(DS_Queue_t *queue, void *data);

void *DS_Queue_dequeue(DS_Queue_t *queue);

bool DS_Queue_destroy(DS_Queue_t *queue, DS_delete_cb del);

#endif /* endif DS_QUEUE_H */