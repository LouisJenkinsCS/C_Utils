#include "alloc_check.h"

void *c_utils_logged_malloc(size_t size, struct c_utils_logger *logger, const char *var_name,
	struct c_utils_location location) {
	void *ptr = malloc(size);
	if(!ptr)
		C_UTILS_LOG_ASSERT_AT(logger, location,
			"malloc failed to allocate memory for size: \"%s\" for variable: \"%s\" with error: \"%s\"",
			 size, var_name, strerror(errno));
	return ptr;
}

void *c_utils_logged_calloc(size_t size, struct c_utils_logger *logger, const char *var_name,
	struct c_utils_location location) {
	void *ptr = calloc(1, size);
	if(!ptr)
		C_UTILS_LOG_ASSERT_AT(logger, location,
			"calloc failed to allocate memory for size: \"%s\" for variable: \"%s\" with error: \"%s\"",
			 size, var_name, strerror(errno));

	return ptr;
}

/*
	Wrapper function to realloc; Logs on error, otherwise it will replace the underlying pointer
	with the newly reallocated one.
*/
void *c_utils_logged_realloc(void *data_ptr, size_t size, struct c_utils_logger *logger,
	const char *var_name, struct c_utils_location location) {
	void *ptr = realloc(*(void **)data_ptr, size);
	if(!ptr) {
		C_UTILS_LOG_ASSERT_AT(logger, location,
			"realloc failed to allocate memory for size: \"%s\" for variable: \"%s\" with error: \"%s\"",
			 size, var_name, strerror(errno));
		return NULL;
	}

	*(void **)data_ptr = ptr;
	return ptr;
}