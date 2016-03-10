#ifndef C_UTILS_REF_COUNT_H
#define C_UTILS_REF_COUNT_H

#include <stdbool.h>
#include <stddef.h>

/*
	ref_count is a reference counting wrapper for malloc. It utilizes advanced pointer arithmetic
	to allow reference counting without needing to alter current data structures. It should be
	noted that if the caller must NEVER EVER use the object after decrementing their own count, 
	they should increment the count BEFORE passing to a callee (I.E another thread), and should
	not decrement the count twice.

	As well, the should define C_UTILS_REF_COUNT if they want to use this utility for other
	library projects to allow easy reference counting. 
*/

#if !defined(C_UTILS_REF_COUNT) && !defined(C_UTILS_REF_COUNT_IGNORE_WARNING)
#warning You should "#define C_UTILS_REF_COUNT" before #include "ref_count.h"
#endif

/*
	The meta-data for the reference counting mechanisms. It manages the destructor, 
	the reference count, and the allocated data.
*/
struct c_utils_ref_count;

typedef void (*c_utils_destructor)(void *);

#ifdef NO_C_UTILS_PREFIX
/*
	Functions
*/
#define ref_create(...) c_utils_ref_create(__VA_ARGS__)
#define ref_inc(...) c_utils_ref_inc(__VA_ARGS__)
#define ref_dec(...) c_utils_ref_dec(__VA_ARGS__)
#endif

void *c_utils_ref_create(size_t size, c_utils_destructor dtor);

void c_utils_ref_inc(void *data);

void c_utils_ref_dec(void *data);

#endif /* C_UTILS_REF_COUNT_H */