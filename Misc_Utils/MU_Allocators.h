#include <MU_Logger.h>

/*
* These wrappers are error-checking and logging functions. They do not do anything special besides existing solely for convenience.
*/

void *MU_malloc(size_t size, MU_Logger_t *logger);

void *MU_calloc(size_t amount, size_t size, MU_Logger_t *logger);

void *MU_realloc(void *ptr, size_t size, MU_Logger_t *logger);