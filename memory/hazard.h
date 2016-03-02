#ifndef C_UTILS_HazardS_H
#define C_UTILS_HazardS_H

#include <stdbool.h>

#ifdef NO_C_UTILS_PREFIX
#define hazard_acquire(...) c_utils_hazard_acquire(__VA_ARGS__)
#define hazard_release(...) c_utils_hazard_release(__VA_ARGS__)
#define hazard_release_all(...) c_utils_hazard_release_all(__VA_ARGS__)
#define hazard_register_destructor(...) c_utils_hazard_register_destructor(__VA_ARGS__)
#endif

#ifdef C_UTILS_HAZARD_MAX_THREAD_COUNT
#define C_UTILS_HAZARD_THREADS C_UTILS_HAZARD_MAX_THREAD_COUNT
#else
#define C_UTILS_HAZARD_THREADS 4
#endif

#ifdef C_UTILS_HAZARD_MAX_PER_THREAD
#define C_UTILS_HAZARD_PER_THREAD C_UTILS_HAZARD_MAX_PER_THREAD
#else
#define C_UTILS_HAZARD_PER_THREAD 4
#endif

/*
	Tags the pointer, ptr, as being in-use, at the given index, and hence will not be
	freed until no further references exist of it.
*/
bool c_utils_hazard_acquire(unsigned int index, void *ptr);

/*
	Registers a destructor which will be called for each ptr after they are
	no longer referenced by any threads. By default, free will be called.
*/
bool c_utils_hazard_register_destructor(void (*destructor)(void *));

/*
	Releases the ptr, and is thereby free to be retired if retire is passed as
	true. A retired pointer is added to an internal list which keeps track of other
	retired pointers. When there no longer is a reference to a retired pointer, it will
	be freed using the hazard table's destructor.
*/
bool c_utils_hazard_release(void *data, bool retire);

/*
	Releases all ptrs, much like c_utils_hazard_release.
*/
bool c_utils_hazard_release_all(bool retire);

#endif /* endif C_UTILS_HazardS_H */