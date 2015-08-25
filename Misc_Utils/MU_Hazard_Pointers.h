#ifndef MU_HAZARD_POINTERS_H
#define MU_HAZARD_POINTERS_H

#include <MU_Logger.h>
#include <DS_List.h>
#include <stdint.h>
#include <stdatomic.h>

#ifdef MU_HAZARD_POINTERS_MAX_THREAD_COUNT
#define MU_HAZARD_POINTERS_MAX_THREADS MU_HAZARD_POINTERS_MAX_THREAD_COUNT
#else
#define MU_HAZARD_POINTERS_MAX_THREADS 8
#endif

#ifdef MU_HAZARD_POINTERS_MAX_PER_THREAD
#define MU_HAZARD_POINTERS_PER_THREAD MU_HAZARD_POINTERS_MAX_PER_THREAD
#else
#define MU_HAZARD_POINTERS_PER_THREAD 4
#endif

typedef struct MU_Hazard_Pointer_t MU_Hazard_Pointer_t;

struct MU_Hazard_Pointer_t{
	_Atomic bool in_use;
	DS_List_t *retired;
	void *hazard_pointers[MU_HAZARD_POINTERS_PER_THREAD];
	struct MU_Hazard_Pointer_t *next;
};

typedef struct {
	MU_Hazard_Pointer_t *head;
	_Atomic size_t size;
	void (*destructor)(void *);
} MU_Hazard_Pointer_List_t;

MU_Hazard_Pointer_List_t *MU_Hazard_Pointer_init(void (*destructor)(void *));

MU_Hazard_Pointer_t *MU_Hazard_Pointer_get(MU_Hazard_Pointer_List_t *hp);

void MU_Hazard_Pointer_acquire(MU_Hazard_Pointer_List_t *list, MU_Hazard_Pointer_t *hp, void *data);

void MU_Hazard_Pointer_release(MU_Hazard_Pointer_List_t *list, MU_Hazard_Pointer_t *hp, void *data);

void MU_Hazard_Pointer_release_all(MU_Hazard_Pointer_List_t *list, MU_Hazard_Pointer_t *hp);

#endif /* endif MU_HAZARD_POINTERS_H */