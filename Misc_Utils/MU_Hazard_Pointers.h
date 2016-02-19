#ifndef MU_HAZARD_POINTERS_H
#define MU_HAZARD_POINTERS_H

#include <MU_Logger.h>
#include <DS_List.h>

#ifdef MU_HAZARD_POINTERS_MAX_THREAD_COUNT
#define MU_HAZARD_POINTERS_MAX_THREADS MU_HAZARD_POINTERS_MAX_THREAD_COUNT
#else
#define MU_HAZARD_POINTERS_MAX_THREADS 4
#endif

#ifdef MU_HAZARD_POINTERS_MAX_PER_THREAD
#define MU_HAZARD_POINTERS_PER_THREAD MU_HAZARD_POINTERS_MAX_PER_THREAD
#else
#define MU_HAZARD_POINTERS_PER_THREAD 4
#endif

typedef struct MU_Hazard_Pointer_t MU_Hazard_Pointer_t;

struct MU_Hazard_Pointer_t{
	volatile bool in_use;
	DS_List_t *retired;
	void *owned[MU_HAZARD_POINTERS_PER_THREAD];
	void (*destructor)(void *);
	struct MU_Hazard_Pointer_t *next;
};

bool MU_Hazard_Pointer_acquire(unsigned int index, void *ptr);

bool MU_Hazard_Pointer_register_destructor(void (*destructor)(void *));

bool MU_Hazard_Pointer_release(void *data, bool retire);

bool MU_Hazard_Pointer_release_all(bool retire);

#endif /* endif MU_HAZARD_POINTERS_H */