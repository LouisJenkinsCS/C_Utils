#ifndef C_UTILS_REF_COUNT_H
#define C_UTILS_REF_COUNT_H

#include <stdbool.h>
#include <stddef.h>

#include "../io/logger.h"

/*
	ref_count is a reference counting wrapper for malloc. It utilizes advanced pointer arithmetic
	to allow reference counting without needing to alter current data structures. It should be
	noted that if the caller must NEVER EVER use the object after decrementing their own count, 
	they should increment the count BEFORE passing to a callee (I.E another thread), and should
	not decrement the count twice.
*/

/*
	The meta-data for the reference counting mechanisms. It manages the destructor, 
	the reference count, and the allocated data.
*/
struct c_utils_ref_count;

struct c_utils_ref_count_conf {
	/// The initial reference count.
	unsigned int initial_ref_count;
	/// Destructor called on once ref_count is below 0.
	void (*destructor)(void *);
	/// Trace logging for reference count changes and destruction.
	struct c_utils_logger *logger;
};

typedef void (*c_utils_destructor)(void *);

#define C_UTILS_REF_INC(data) _c_utils_ref_inc(data, C_UTILS_LOCATION)

#define C_UTILS_REF_DEC(data) _c_utils_ref_dec(data, C_UTILS_LOCATION)

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_ref_count_conf ref_count_conf_t;

/*
	Functions
*/
#define ref_create(...) c_utils_ref_create(__VA_ARGS__)
#define ref_create_conf(...) c_utils_ref_create_conf(__VA_ARGS__)
#define ref_inc(...) c_utils_ref_inc(__VA_ARGS__)
#define ref_dec(...) c_utils_ref_dec(__VA_ARGS__)
#endif

/*
	TODO: Create a macro for this instead, so when we wish to increment and decrement, it will automatically pass in
	the __LINE__, __FILE__, and __FUNCTION__ it is being called from.
*/

void *c_utils_ref_create(size_t size);

void *c_utils_ref_create_conf(size_t size, struct c_utils_ref_count_conf *conf);

void _c_utils_ref_inc(void *data, struct c_utils_location log_info);

void _c_utils_ref_dec(void *data, struct c_utils_location log_info);

void c_utils_ref_destroy(void *data);

#endif /* C_UTILS_REF_COUNT_H */