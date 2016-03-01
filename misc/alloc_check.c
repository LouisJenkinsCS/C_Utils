#include "alloc_check.h"

void *c_utils_logged_malloc(size_t size, struct c_utils_logger *logger, const char *var_name,
	const char *line, const char *function, const char *file) {
	void *ptr = malloc(size);
	if(!ptr)
		c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ASSERTION, NULL,
			"malloc failed to allocate memory for size: \"%s\" for variable: \"%s\" with error: \"%s\"",
			NULL, file, line, function, size, var_name, strerror(errno));

	return ptr;
}

void *c_utils_logged_calloc(size_t size, struct c_utils_logger *logger, const char *var_name,
	const char *line, const char *function, const char *file) {
	void *ptr = calloc(1, size);
	if(!ptr)
		c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ASSERTION, NULL,
			"calloc failed to allocate memory for size: \"%s\" for variable: \"%s\" with error: \"%s\"",
			NULL, file, line, function, size, var_name, strerror(errno));

	return ptr;
}

/*
	Wrapper function to realloc; Logs on error, otherwise it will replace the underlying pointer
	with the newly reallocated one.
*/
void *c_utils_logged_realloc(void *data_ptr, size_t size, struct c_utils_logger *logger,
	const char *var_name, const char *line, const char *function, const char *file) {
	void *ptr = realloc(*(void **)data_ptr, size);
	if(!ptr) {
		c_utils_logger_log(logger, C_UTILS_LOG_LEVEL_ASSERTION, NULL,
			"realloc failed to allocate memory for size: \"%s\" for variable: \"%s\" with error: \"%s\"",
			NULL, file, line, function, size, var_name, strerror(errno));
		return NULL;
	}

	*(void **)data_ptr = ptr;
	return ptr;
}