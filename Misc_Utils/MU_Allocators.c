#include <MU_Allocators.h>

void *MU_malloc(size_t size, MU_Logger_t *logger){
	if(!size) return NULL;
	void *ptr = malloc(size);
	if(!ptr){
		MU_LOG_ASSERT(logger, "MU_malloc->malloc: \"%s\"\n", strerror(errno));
	}
	return ptr;
}

void *MU_calloc(size_t amount, size_t size, MU_Logger_t *logger){
	if(!size) return NULL;
	void *ptr = calloc(amount, size);
	if(!ptr){
		MU_LOG_ASSERT(logger, "MU_calloc->calloc: \"%s\"\n", strerror(errno));
	}
	return ptr;
}

void *MU_realloc(void *ptr, size_t size, MU_Logger_t *logger){
	if(!ptr || !size) return NULL;
	void tmp_ptr = realloc(ptr, size);
	if(!tmp_ptr){
		MU_LOG_ASSERT(logger, "MU_realloc->realloc: \"%s\"\n", strerror(errno));
	}
	return tmp_ptr;
}