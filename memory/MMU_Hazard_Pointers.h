#ifndef MMU_HazardS_H
#define MMU_HazardS_H

#include <MU_Logger.h>
#include <DS_List.h>

#ifdef C_UTILS_USE_POSIX_STD
#define hazard_acquire(...) MMU_Hazard_acquire(__VA_ARGS__)
#define hazard_release(...) MMU_Hazard_release(__VA_ARGS__)
#define hazard_release_all(...) MMU_Hazard_release_all(__VA_ARGS__)
#define hazard_register_destructor(...) MMU_Hazard_register_destructor(__VA_ARGS__)
#endif

#ifdef MMU_HAZARDS_MAX_THREAD_COUNT
#define MMU_HAZARDS_MAX_THREADS MMU_HazardS_MAX_THREAD_COUNT
#else
#define MMU_HAZARDS_MAX_THREADS 4
#endif

#ifdef MMU_HAZARDS_MAX_PER_THREAD
#define MMU_HAZARDS_PER_THREAD MMU_HazardS_MAX_PER_THREAD
#else
#define MMU_HAZARDS_PER_THREAD 4
#endif

typedef struct MMU_Hazard_t MMU_Hazard_t;

struct MMU_Hazard_t{
	volatile bool in_use;
	size_t id;
	DS_List_t *retired;
	void *owned[MMU_HazardS_PER_THREAD];
	void (*destructor)(void *);
	struct MMU_Hazard_t *next;
};

/*
	Tags the pointer, ptr, as being in-use, at the given index, and hence will not be
	freed until no further references exist of it.
*/
bool MMU_Hazard_acquire(unsigned int index, void *ptr);

/*
	Registers a destructor which will be called for each ptr after they are
	no longer referenced by any threads. By default, free will be called.
*/
bool MMU_Hazard_register_destructor(void (*destructor)(void *));

/*
	Releases the ptr, and is thereby free to be retired if retire is passed as
	true. A retired pointer is added to an internal list which keeps track of other
	retired pointers. When there no longer is a reference to a retired pointer, it will
	be freed using the hazard table's destructor.
*/
bool MMU_Hazard_release(void *data, bool retire);

/*
	Releases all ptrs, much like MMU_Hazard_release.
*/
bool MMU_Hazard_release_all(bool retire);

#endif /* endif MMU_HazardS_H */