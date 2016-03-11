#include "ref_count.h"

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>

struct c_utils_ref_count {
	// Destructor called when ref_count is 0
	c_utils_destructor dtor;
	// Atomic reference counter
	_Atomic int counter;
	// Pointer to allocated data region outside of this struct
	void *ptr;
};

/*
	Using pointer arithmetic we can obtain the start of the struct the ptr was allocated after,
	being the ref_count.
*/
static struct c_utils_ref_count *get_ref_count_from(void *ptr) {
	return ptr - sizeof(struct c_utils_ref_count);
}

void *c_utils_ref_create(size_t size, c_utils_destructor dtor) {
	// Note we allocate more than just enough for the ref_count.
	struct c_utils_ref_count *rc = malloc(sizeof(*rc) + size);
	if(!rc)
		return NULL;

	rc->dtor = dtor;
	rc->counter = ATOMIC_VAR_INIT(0);
	// Points to the end of the struct, the data allocated after ref_count
	rc->ptr = rc + 1;

	return rc;
}

void c_utils_ref_inc(void *ptr) {
	// Assure data is not null.
	assert(ptr);

	/*
		Some things should be said about this. For one, if ptr is not the same
		pointer as was returned originally, there is a possibility of a segmentation
		fault. That is true for anything at all, however by making this check, we
		can avoid undefined behavior almost entirely by checking if at the offset
		of ptr in the region of data is the same as ptr, it is safe to continue.

		However, if they are NOT equal, then we fail the assertion. The significance of this is
		that if the ptr is still inside this heap, then we have a high chance of avoing data corruption.

		So, in summary, worst case scenario, segmentation fault due to reading outside of the boundary of the
		heap... best case scenario, we fail assertion and we know exactly why.
	*/
	struct c_utils_ref_count *rc = get_ref_count_from(ptr);
	assert(rc->ptr == ptr);

	atomic_fetch_add(&rc->counter, 1);
}

void c_utils_ref_dec(void *ptr) {
	assert(ptr);

	struct c_utils_ref_count *rc = get_ref_count_from(ptr);
	assert(rc->ptr == ptr);

	int refs;
	// If the count is already 0 (since it fetches old value first) we fail assertion.
	assert((refs = atomic_fetch_sub(&rc->counter, 1)));

	/*
		Note that if a thread attempts to increment after count is 0, this race condition invoked undefined behavior.
		Hence it is up to the caller. The assertions just make finding the errors easier, the edge cases where something
		increments the count after we succeed the assertion and enter the if condition, is a result of the failure on the caller.
	*/
	if(!refs) {
		rc->dtor(ptr);
		free(rc);
	}
}