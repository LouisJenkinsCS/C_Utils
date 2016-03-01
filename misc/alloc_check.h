#ifndef C_UTILS_ALLOC_CHECK_H
#define C_UTILS_ALLOC_CHECK_H

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../io/logger.h"

void *c_utils_logged_malloc(size_t size, struct c_utils_logger *logger, const char *var_name,
	const char *line, const char *function, const char *file);

void *c_utils_logged_calloc(size_t size, struct c_utils_logger *logger, const char *var_name,
	const char *line, const char *function, const char *file);

/*
	Wrapper function to realloc; Logs on error, otherwise it will replace the underlying pointer
	with the newly reallocated one.
*/
void *c_utils_logged_realloc(void *data_ptr, size_t size, struct c_utils_logger *logger,
	const char *var_name, const char *line, const char *function, const char *file);


/*
	Uses the one-shot for-loop macro trick I used in scoped_lock. This time,
	it allocates the passed variable, and internally (function defined above),
	it will log any errors. When NULL is returned, it will run the block. Otherwise,
	it will skip it entirely. This is used to placate the need to log every single
	allocation and check each time. Now, I can just do the following...

	struct c_utils_list *list;
	C_UTILS_ON_BAD_MALLOC(list, logger, sizeof(*list))
		return NULL;

	This may look strange at first, but not really once you analyze it. It also allows
	for really complex statements as well.

	C_UTILS_ON_BAD_MALLOC(thread_pool, logger, sizeof(*thread_pool)) {
		free_all_other_resources();
		close_stream();
		do_some_stuff();
		return NULL;
	}
	
	Of course that can be done in a goto statement, but this also makes it easier for
	those who swear up and down goto statements should never be used.

	The allocation wrapper also logs the file, function and line number, which is
	crucial for determining when and where a bad allocation occurs.

	The below are very boilerplate, but besides for malloc and calloc, realloc requires
	a bit of a different template. Hence, all of them are very similar.
*/
#define C_UTILS_ON_BAD_MALLOC(var, logger, size) \
	for(void *_tester = var = c_utils_logged_malloc(size, logger, \
		C_UTILS_STRINGIFY(var), C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, \
		__FILE__); !_tester; _tester++)

#define C_UTILS_ON_BAD_CALLOC(var, logger, size) \
	for(void *_tester = var = c_utils_logged_calloc(size, logger, \
		C_UTILS_STRINGIFY(var), C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, \
		__FILE__); !_tester; _tester++)

#define C_UTILS_ON_BAD_REALLOC(var_ptr, logger, size) \
	for(void *_tester = c_utils_logged_realloc(var_ptr, size, logger, \
		C_UTILS_STRINGIFY(var), C_UTILS_STRINGIFY(__LINE__), __FUNCTION__, \
		__FILE__); !_tester; _tester++)

#endif /* C_UTILS_ALLOC_CHECK_H */