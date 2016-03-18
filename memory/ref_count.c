#include "ref_count.h"

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>

struct c_utils_ref_count {
	/// Atomic reference counter
	_Atomic int refs;
	/// Pointer to allocated data region outside of this struct
	void *data;
	/// Configuration
	struct c_utils_ref_count_conf conf;
};

/*
	Using pointer arithmetic we can obtain the start of the struct the ptr was allocated after,
	being the ref_count.
*/
static struct c_utils_ref_count *get_ref_count_from(void *ptr) {
	return ptr - sizeof(struct c_utils_ref_count);
}

void *c_utils_ref_create(size_t size) {
	struct c_utils_ref_count_conf conf = {};
	return c_utils_ref_create_conf(size, &conf);
}

void *c_utils_ref_create_conf(size_t size, struct c_utils_ref_count_conf *conf) {
	if(!conf)
		return NULL;

	// Note we allocate more than just enough for the ref_count.
	struct c_utils_ref_count *rc = malloc(sizeof(*rc) + size);
	if(!rc)
		return NULL;

	rc->conf = *conf;
	if(!rc->conf.destructor)
		rc->conf.destructor = free;

	rc->refs = ATOMIC_VAR_INIT(conf->initial_ref_count);
	// Points to the end of the struct, the data allocated after ref_count
	rc->data = rc + 1;

	C_UTILS_LOG_TRACE(conf->logger, "An object of size %zu was allocated with an initial reference count of %u", size, conf->initial_ref_count);
	return rc->data;
}

void _c_utils_ref_inc(void *ptr, struct c_utils_location log_info) {
	// Assure data is not null.
	assert(ptr);

	/*
		Some things should be said about this. For one, if ptr is not the same
		pointer as was returned originally, there is a possibility of a segmentation
		fault. That is true for anything at all, however by making this check, we
		can avoid undefined behavior almost entirely by checking if at the offset
		of data in ref_count is the same as ptr, it is safe to continue.

		However, if they are NOT equal, then we fail the assertion. The significance of this is
		that if the ptr is still inside this heap, then we have a high chance of avoing data corruption.

		So, in summary, worst case scenario, segmentation fault due to reading outside of the boundary of the
		heap... best case scenario, we fail assertion and we know exactly why.
	*/
	struct c_utils_ref_count *rc = get_ref_count_from(ptr);
	assert(rc->data == ptr);

	int refs = atomic_fetch_add(&rc->refs, 1);

	C_UTILS_LOG_TRACE_AT(rc->conf.logger, log_info,  "Reference count was incremented from %d to %d", refs, refs + 1);
}

void _c_utils_ref_dec(void *ptr, struct c_utils_location log_info) {
	assert(ptr);

	struct c_utils_ref_count *rc = get_ref_count_from(ptr);
	assert(rc->data == ptr);

	int refs;
	// If the count is already 0 (since it fetches old value first) we fail assertion.
	assert((refs = atomic_fetch_sub(&rc->refs, 1)) > -1);

	C_UTILS_LOG_TRACE_AT(rc->conf.logger, log_info, "Reference count was decremented from %d to %d", refs, refs - 1);
	
	/*
		Note that if a thread attempts to increment after count is 0, this race condition invoked undefined behavior.
		Hence it is up to the caller. The assertions just make finding the errors easier, the edge cases where something
		increments the count after we succeed the assertion and enter the if condition, is a result of the failure on the caller.
	*/
	if(!refs) {
		C_UTILS_LOG_TRACE_AT(rc->conf.logger, log_info, "Reference count reached below 0, destroying object...");
		rc->conf.destructor(ptr);
		free(rc);
	}
}