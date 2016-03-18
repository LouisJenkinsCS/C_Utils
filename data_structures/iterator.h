#ifndef DS_ITERATOR_H
#define DS_ITERATOR_H

#include <stdbool.h>

#include "helpers.h"
#include "../io/logger.h"

struct c_utils_iterator_conf {
	/// If we hold a reference count to the handle.
	bool ref_counted;
};

struct c_utils_iterator {
	/// Handle for the data structure being iterated over.
	void *handle;
	/// The relative position
	void *pos;
	/*
		Below are callbacks that may be filled out by the data structure that creates
		an instance of this object.
	*/
	void *(*head)(void *handle, void *pos);
	void *(*tail)(void *handle, void *pos);
	void *(*next)(void *handle, void *pos);
	void *(*prev)(void *handle, void *pos);
	void *(*curr)(void *handle, void *pos);
	bool (*append)(void *handle, void *pos, void *item);
	bool (*prepend)(void *handle, void *pos, void *item);
	bool (*for_each)(void *handle, void *pos, c_utils_general_cb);
	bool (*rem)(void *handle, void *pos);
	bool (*del)(void *handle, void *pos);
	void (*finalize)(void *handle, void *pos);
	// Configuration
	struct c_utils_iterator_conf conf;
};

void c_utils_auto_destroy_iterator(struct c_utils_iterator **it);

#define C_UTILS_AUTO_ITERATOR struct c_utils_iterator *__attribute__ ((__cleanup__(c_utils_auto_destroy_iterator)))

#define C_UTILS_ITERATOR_FOR_EACH(tmp_var, it) for(C_UTILS_AUTO_ITERATOR auto_it = it; (tmp_var = c_utils_iterator_next(auto_it));)


#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_iterator iterator_t;

/*
	Macros
*/
#define AUTO_ITERATOR C_UTILS_AUTO_ITERATOR

/*
	Functions
*/
#define iterator_head(...) c_utils_iterator_head(__VA_ARGS__)
#define iterator_tail(...) c_utils_iterator_tail(__VA_ARGS__)
#define iterator_next(...) c_utils_iterator_next(__VA_ARGS__)
#define iterator_prev(...) c_utils_iterator_prev(__VA_ARGS__)
#define iterator_curr(...) c_utils_iterator_curr(__VA_ARGS__)
#define iterator_append(...) c_utils_iterator_append(__VA_ARGS__)
#define iterator_prepend(...) c_utils_iterator_prepend(__VA_ARGS__)
#define iterator_remove(...) c_utils_iterator_remove(__VA_ARGS__)
#define iterator_delete(...) c_utils_iterator_delete(__VA_ARGS__)
#define iterator_for_each(...) c_utils_iterator_for_each(__VA_ARGS__)
#define iterator_destroy(...) c_utils_iterator_destroy(__VA_ARGS__)
#endif

/*
	The iterator works by invoking callbacks specified by the creator (I.E, the C_Utils data structures)
	and if one is not specified (left as NULL), we automatically fail the call. This allows one to use
	the iterator generically for any data structure, so long as they support the call.

	The iterator can optionally maintain a reference count to the underlying data structure to ensure that
	it does not become invalid between calls, before the iterator is actually destroyed. The iterator supports
	custom macros for iterating through each item in the list, however more often than not, the implementor have
	macros which are more specific and should be used instead.

	Iterators support passing a custom type, of which can  be checked to ensure the iterator is compatible with
	their helper macros. For example, in C_Util's concurrent map, it supports iterating over the map by
	keys, values, and key-value pairs; none of which would work on one returned from list. This allows generic but
	safe iterators to prevent the invokation of unintentional undefined behavior.

	To summarize when you would use the iterator macro over the for_each, or even iterator_for_each, remember that
	there are some short-comings when it comes having it invoke the callback on each directly. For one, for concurrent
	data structures, this means you acquire the lock for the entire time, which may be good depending on use, it may also
	cause starvation of other threads attempting to access the underlying data structure. For instance, list supports
	concurrent access through a reader-writer lock supplied in the pthread library, named pthread_rwlock_t, and upon which
	calling for_each on a large list can result in writer starvation as it uses the reader-lock. Through the iterator macro,
	it will allow any writers the chance to write to the data structure, making for more efficient, less starved, and possibly
	less contended accesses. Furthermore, there is the convenience of using a block of code on each item rather, say, having to
	create a function just to do a simple procedure. Lastly, you cannot remove an item from the list while the reader-lock is
	acquired (as this would dead-lock) with for_each, while you CAN with the iterator macro because you relinquish the lock on each
	call.
*/

void *c_utils_iterator_head(struct c_utils_iterator *it);

void *c_utils_iterator_tail(struct c_utils_iterator *it);

void *c_utils_iterator_next(struct c_utils_iterator *it);

void *c_utils_iterator_prev(struct c_utils_iterator *it);

void *c_utils_iterator_curr(struct c_utils_iterator *it);

bool c_utils_iterator_append(struct c_utils_iterator *it, void *item);

bool c_utils_iterator_prepend(struct c_utils_iterator *it, void *item);

bool c_utils_iterator_for_each(struct c_utils_iterator *it, c_utils_general_cb cb);

bool c_utils_iterator_remove(struct c_utils_iterator *it);

bool c_utils_iterator_delete(struct c_utils_iterator *it);

void c_utils_iterator_destroy(struct c_utils_iterator *it);

#endif /* endif DS_ITERATOR_H */
