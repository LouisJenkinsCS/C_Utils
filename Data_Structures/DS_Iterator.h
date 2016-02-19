#ifndef DS_ITERATOR_H
#define DS_ITERATOR_H

#include <stdbool.h>
#include <DS_Helpers.h>
#include <MU_Logger.h>

/*
	The below structure is an early prototype on my own implementation of a generic iterator.
	By using an anonymous/opaque handle, it allows me to generalize it's use, as well as 
	maintain complete flexibility. The big problem being there is no way to validate if the
	handle is still valid, but that is true for all non-kernel data structures, hence it is
	up to the user to not destroy the data structure being pointed to and attempt to dereference it.

	The structure is callback heavy, and all callbacks should be filled by the data structures which
	creates this structure, and the callbacks themselves should not be called by the user, only through
	the abstraction layer supplied as the API. The callbacks themselves can be rather tricky, but are
	relatively simple. They all pass the ds_handle to the callback, meaning any data structure can
	implement this, generically. Then it takes a pointer to the current node, also maintained 
	by the iterator itself, and should be checked to see if it is still valid, hence all operations
	should be O(n), even if it's a simple operation such as next and prev. Lastly, except for del which
	takes an obvious, with no need to mention, deletion callback, in which case is it's 4th, the 3rd parameter
	is a pointer to the desired storage to store the result of the transaction. 
*/

/// Struct used to help the data structure hint at it's current position.
struct Position_Hint {
	DS_Node_t *curr;
	DS_Node_t *next;
	DS_Node_t *prev;
};

typedef struct {
	/// Handle for the data structure being iterated over.
	void *ds_handle;
	/// The relative position
	struct Position_Hint pos;
	/*
		Below are callbacks that may be filled out by the data structure that creates
		an instance of this object.
	*/
	void *(*head)(void *, struct Position_Hint *);
	void *(*tail)(void *, struct Position_Hint *);
	void *(*next)(void *, struct Position_Hint *);
	void *(*prev)(void *, struct Position_Hint *);
	bool (*append)(void *, struct Position_Hint *, void *);
	bool (*prepend)(void *, struct Position_Hint *, void *);
	bool (*for_each)(void *, struct Position_Hint *, DS_general_cb);
	bool (*del)(void *, struct Position_Hint *, DS_delete_cb);
} DS_Iterator_t;

void *DS_Iterator_head(DS_Iterator_t *it);

void *DS_Iterator_tail(DS_Iterator_t *it);

void *DS_Iterator_next(DS_Iterator_t *it);

void *DS_Iterator_prev(DS_Iterator_t *it);

bool DS_Iterator_append(DS_Iterator_t *it, void *item);

bool DS_Iterator_prepend(DS_Iterator_t *it, void *item);

bool DS_Iterator_for_each(DS_Iterator_t *it, DS_general_cb cb);

bool DS_Iterator_remove(DS_Iterator_t *it, DS_delete_cb del);

#endif /* endif DS_ITERATOR_H */