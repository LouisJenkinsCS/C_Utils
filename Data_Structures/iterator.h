#ifndef DS_ITERATOR_H
#define DS_ITERATOR_H

#include <stdbool.h>

#include "helpers.h"
#include "../io/logger.h"

/// Struct used to help the data structure hint at it's current position.
struct c_utils_position {
	struct c_utils_node *curr;
	struct c_utils_node *next;
	struct c_utils_node *prev;
};

struct c_utils_iterator {
	/// Handle for the data structure being iterated over.
	void *handle;
	/// The relative position
	struct c_utils_position pos;
	/*
		Below are callbacks that may be filled out by the data structure that creates
		an instance of this object.
	*/
	void *(*head)(void *, struct c_utils_position *);
	void *(*tail)(void *, struct c_utils_position *);
	void *(*next)(void *, struct c_utils_position *);
	void *(*prev)(void *, struct c_utils_position *);
	bool (*append)(void *, struct c_utils_position *, void *);
	bool (*prepend)(void *, struct c_utils_position *, void *);
	bool (*for_each)(void *, struct c_utils_position *, c_utils_general_cb);
	bool (*del)(void *, struct c_utils_position *, c_utils_delete_cb);
};

void c_utils_auto_destroy_iterator(struct c_utils_iterator **it){
	free(*it);
}

#define C_UTILS_AUTO_ITERATOR struct c_utils_iterator *__attribute__ ((__cleanup__(c_utils_auto_destroy_iterator)))

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_iterator iterator_t;

/*
	Functions
*/
#define iterator_head(...) c_utils_iterator_head(__VA_ARGS__)
#define iterator_tail(...) c_utils_iterator_tail(__VA_ARGS__)
#define iterator_next(...) c_utils_iterator_next(__VA_ARGS__)
#define iterator_prev(...) c_utils_iterator_prev(__VA_ARGS__)
#define iterator_append(...) c_utils_iterator_append(__VA_ARGS__)
#define iterator_prepend(...) c_utils_iterator_prepend(__VA_ARGS__)
#define iterator_remove(...) c_utils_iterator_remove(__VA_ARGS__)
#define iterator_for_each(...) c_utils_iterator_for_each(__VA_ARGS__)
#endif

/*
	The below structure is an early prototype on my own implementation of a generic iterator.
	By using an anonymous/opaque handle, it allows me to generalize it's use, as well as 
	maintain complete flexibility. The big problem being there is no way to validate if the
	handle is still valid, but that is true for all non-kernel data structures, hence it is
	up to the user to not destroy the data structure being pointed to and attempt to dereference it.

	The structure is callback heavy, and all callbacks should be filled by the data structures which
	creates this structure, and the callbacks themselves should not be called by the user, only through
	the abstraction layer supplied as the API. The callbacks themselves can be rather tricky, but are
	relatively simple. They all pass the handle to the callback, meaning any data structure can
	implement this, generically. Then it takes a pointer to the current node, also maintained 
	by the iterator itself, and should be checked to see if it is still valid, hence all operations
	should be O(n), even if it's a simple operation such as next and prev. Lastly, except for del which
	takes an obvious, with no need to mention, deletion callback, in which case is it's 4th, the 3rd parameter
	is a pointer to the desired storage to store the result of the transaction. 
*/

void *c_utils_iterator_head(struct c_utils_iterator *it){
	if (!it || !it->head) return NULL;
	return it->head(it->handle, &it->pos);
}

void *c_utils_iterator_tail(struct c_utils_iterator *it){
	if (!it || !it->tail) return NULL;
	return it->tail(it->handle, &it->pos);
}

void *c_utils_iterator_next(struct c_utils_iterator *it){
	if (!it || !it->next) return NULL;
	return it->next(it->handle, &it->pos);
}

void *c_utils_iterator_prev(struct c_utils_iterator *it){
	if (!it || !it->prev) return NULL;
	return it->prev(it->handle, &it->pos);
}

bool c_utils_iterator_append(struct c_utils_iterator *it, void *item){
	if (!it || !it->append) return false;
	return it->append(it->handle, &it->pos, item);
}

bool c_utils_iterator_prepend(struct c_utils_iterator *it, void *item){
	if (!it || !it->prepend) return false;
	return it->prepend(it->handle, &it->pos, item);
}

bool c_utils_iterator_for_each(struct c_utils_iterator *it, c_utils_general_cb cb){
	if (!it || !it->for_each) return false;
	return it->for_each(it->handle, &it->pos, cb);
}

bool c_utils_iterator_remove(struct c_utils_iterator *it, c_utils_delete_cb del){
	if (!it || !it->del) return NULL;
	return it->del(it->handle, &it->pos, del);
}

#endif /* endif DS_ITERATOR_H */