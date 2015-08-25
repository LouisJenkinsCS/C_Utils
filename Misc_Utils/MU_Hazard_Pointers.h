#ifndef MU_HAZARD_POINTERS_H
#define MU_HAZARD_POINTERS_H

#include <MU_Logger.h>
#include <DS_List.h>
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

typedef struct {
	MU_Hazard_Pointer_t *head;
	_Atomic size_t size;
} MU_HP_List_t;

void MU_Hazard_Pointer_scan(MU_HP_List_t *list, MU_Hazard_Pointer_t *hp);

void MU_Hazard_Pointer_help_scan(MU_HP_List_t *list, MU_Hazard_Pointer_t *hp);

MU_HP_List_t *MU_HP_List_create();

MU_Hazard_Pointer_t *MU_Hazard_Pointer_create();

MU_Hazard_Pointer_t *MU_Hazard_Pointer_allocate(MU_HP_List_t *hp);

void MU_Hazard_Pointer_retire(MU_Hazard_Pointer_t *hp);

void MU_Hazard_Pointer_release(MU_HP_List_t *list, MU_Hazard_Pointer_t *hp, void *data);

#endif /* endif MU_HAZARD_POINTERS_H */