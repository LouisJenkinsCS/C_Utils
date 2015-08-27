#ifndef MU_HAZARD_POINTERS_H
#define MU_HAZARD_POINTERS_H

#include <MU_Logger.h>
#include <DS_List.h>
#include <stdint.h>
#include <stdatomic.h>

#ifdef MU_HAZARD_POINTERS_MAX_THREAD_COUNT
#define MU_HAZARD_POINTERS_MAX_THREADS MU_HAZARD_POINTERS_MAX_THREAD_COUNT
#else
#define MU_HAZARD_POINTERS_MAX_THREADS 1
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
	void *owned[MU_HAZARD_POINTERS_PER_THREAD];
	void (*destructor)(void *);
	struct MU_Hazard_Pointer_t *next;
};

MU_Hazard_Pointer_t *MU_Hazard_Pointer_acquire(void);

bool MU_Hazard_Pointer_register_destructor(void (*destructor)(void *));

bool MU_Hazard_Pointer_reset(MU_Hazard_Pointer_t *hp);

bool MU_Hazard_Pointer_release(MU_Hazard_Pointer_t *hp);

#endif /* endif MU_HAZARD_POINTERS_H */