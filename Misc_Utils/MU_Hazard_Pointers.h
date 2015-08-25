#ifndef MU_HAZARD_POINTERS_H
#define MU_HAZARD_POINTERS_H

#include <MU_Logger.h>
#include <stdint.h>
#include <stdatomic.h>

#define MU_HAZARD_POINTERS_PER_THREAD 4

typedef struct MU_Hazard_Pointer_t MU_Hazard_Pointer_t;

struct MU_Hazard_Pointer_t{
	_Atomic bool in_use;
	DS_List_t *retired;
	void *hazard_pointers[MU_HAZARD_POINTERS_PER_THREAD];
	struct MU_Hazard_Pointer_t *next;
};

#endif /* endif MU_HAZARD_POINTERS_H */