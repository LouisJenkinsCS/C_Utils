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
<<<<<<< HEAD
=======
	size_t id;
>>>>>>> development
	DS_List_t *retired;
	void *owned[MU_HAZARD_POINTERS_PER_THREAD];
	void (*destructor)(void *);
	struct MU_Hazard_Pointer_t *next;
};

<<<<<<< HEAD
bool MU_Hazard_Pointer_acquire(unsigned int index, void *ptr);

bool MU_Hazard_Pointer_register_destructor(void (*destructor)(void *));

bool MU_Hazard_Pointer_release(void *data, bool retire);

=======
/*
	Tags the pointer, ptr, as being in-use, at the given index, and hence will not be
	freed until no further references exist of it.
*/
bool MU_Hazard_Pointer_acquire(unsigned int index, void *ptr);

/*
	Registers a destructor which will be called for each ptr after they are
	no longer referenced by any threads. By default, free will be called.
*/
bool MU_Hazard_Pointer_register_destructor(void (*destructor)(void *));

/*
	Releases the ptr, and is thereby free to be retired if retire is passed as
	true. A retired pointer is added to an internal list which keeps track of other
	retired pointers. When there no longer is a reference to a retired pointer, it will
	be freed using the hazard table's destructor.
*/
bool MU_Hazard_Pointer_release(void *data, bool retire);

/*
	Releases all ptrs, much like MU_Hazard_Pointer_release.
*/
>>>>>>> development
bool MU_Hazard_Pointer_release_all(bool retire);

#endif /* endif MU_HAZARD_POINTERS_H */