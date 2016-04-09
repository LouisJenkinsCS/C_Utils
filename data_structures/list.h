#ifndef C_UTILS_LIST_H
#define C_UTILS_LIST_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "iterator.h"
#include "helpers.h"
#include "../io/logger.h"

/*
	Will use a Reader-Writer lock to allow concurrent access to the list. The writer lock, of course, will be
	used whenever a mutating operation goes on, and a concurrent reader-locked used otherwise to allow parallel
	access to the list.
*/
#define C_UTILS_LIST_CONCURRENT 1 << 0

/* 
	This will create the list with a reference count with the appropriate destructor for this type.
	This allows an iterator to remain valid even after all other users relinquish their count.
*/
#define C_UTILS_LIST_RC_INSTANCE 1 << 1

/*
	This REQUIRES that the item has been created WITH a reference count. This is used to increment the reference
	count when used by the list, or held by the iterator, and decrement when it is finished.
*/
#define C_UTILS_LIST_RC_ITEM 1 << 2

/*
	Marks the list to call it's destructor on each item when it is destroyed. Hence, you MUST know ahead of time if you are going
	to have this functionality. Of course, you can first remove_all/delete_all and then destroy the list without this if need be.
*/
#define C_UTILS_LIST_DELETE_ON_DESTROY 1 << 3

/*
	A double linked-list implementation, which can used as a generic data structure. 

	The linked list allows custom configuration through list_conf_t objects, which act
	as configuration objects for the linked list. Through this, the list can be a
	ordered, if cmp is specified; a concurrent and thread-safe list through the protection
	of a reader-writer lock (pthread_rwlock_t); verbose logging of information through 
	the custom logger, for trace logging and debugging errors; deleting items in the list
	if del is specified; deleting all items on destruction if del_items_on_free is specified.

	To get on to the more interesting part, this list is concurrent, allowing multiple readers to
	safely access the list. This is most notable with the iterator implementation, allowing for
	easy concurrent reads without directly impeding performance over the direct callback for_each
	function.

	Time complexity of all functions are in fact O(N), with the exception for list_add, which if
	there is no comparator, it will be O(1).

	Time complexity of the iterator functions are also O(N), with the exceptions of iterator_append and
	iterator_prepend, which are O(1) at that position. I say that they are O(N) because, functions
	iterator_next and iterator_prev, while they iterate in an O(1) fashion, iterating through the
	entirety of the list is O(N).

	Head:
		Concurrent:
			Yes
		Complexity:
			O(1)
		Notes:
			Advances the iterator to the head of the list.
	Tail:
		Concurrent:
			Yes
		Complexity:
			O(1)
		Notes:
			Advances the iterator to the tail of the list.
	Next:
 		Concurrent:
 			Yes
 		Complexity:
 			O(1)
		Notes:
			If the current node is invalidated, it will first check if next is valid, and then if prev is valid.
			If next is valid, it will just advance to next, which would have been the next item in the list had
			it not been invalidated anyway. If prev is valid, it will jump to prev->next to attempt to bridge
			the gap. If neither are valid, the iterator is in an invalidated state and the current position is
			reset and it will return a failure.
	Prev:
		Concurrent:
			Yes
		Complexity:
			O(1)
		Notes:
			If the current node is invalidated, it will first check if prev is valid, and then if next is valid.
			If prev is valid, it will just advance to prev, which would have been the previous item in the list had
			it not been invalidated anyway. If next is invalid, it will jump to next->prev to attempt to bridge
			the gap. If neither are valid, the iterator is in an invalidated state and the current position is
			reset and it will return a failure.
	Curr:
		Concurrent:
			Yes
		Complexity:
			O(1)
		Notes:
			Obtains the last item the iterator has iterated over if it is still valid.
*/

struct c_utils_list;

/**
 *	flags:
 *		defaults:
 *			0
 *		notes:
 *			Used to toggle certain functionality on and off.
 *	callbacks:
 *		comparator:
 *			defaults:
 *				NULL
 *			notes:
 *				When specified, the list will become an ordered list. This functionality disables the append and prepend
 *				features of the iterator for this given instance, as it will tamper with the order of the list.
 *		destructor
 *			defaults:
 *				free
 *			notes:
 *				When specified, the list will call this when delete is called (instead of remove). Note that, this will ultimately
 *				not be used if flag LIST_RC_ITEM is specified as it will instead unreference the item instead, in both remove and delete.
 *	logger:
 *		defaults:
 *			NULL
 *		notes:
 *			Can be used to allow tracing and debugging information. This logger will also be passed to the reference counter meta data if the
 *			flag LIST_RC_INSTANCE.
 */
struct c_utils_list_conf {
	/// Additional flags used to configure and tune the list.
	int flags;
	/// Grouping of callback functions
	struct {
		struct {
			int (*item)(const void *, const void *);
		} comparators;
		struct {
			void (*item)(void *);
		} destructors;
	} callbacks;
	struct {
		size_t max;
	} size;
	/// Used to log any errors or trace information to.
	struct c_utils_logger *logger;
};

/*
	Used to iterate through the list using an automatic iterator (requires GCC and Clang) and is optimized for concurrent access.
	The iterator, of course, always be valid so long as the correct flags are passed, I.E LIST_RC_INSTANCE to maintain a reference
	count to the list, and LIST_RC_ITEM to keep a reference count to the item. The former being obvious, but the latter coming extremely
	in handy when you have other threads removing items from the list while iterating (hence, even though when it was retrieved the item
	was valid, it is possible that after, when we relinquish the lock, it gets destroyed). The alternative to this is the internal
	list_for_each function relying on callbacks, which would prevent such things.
*/
#define C_UTILS_LIST_FOR_EACH(item, list) \
	for(C_UTILS_AUTO_ITERATOR _this_iterator = c_utils_list_iterator(list); (item = c_utils_iterator_next(_this_iterator));)

/*
	Used to iterate backwards through the list using an automatic iterator (requires GCC and Clang) and is optimized for concurrent access.
	The iterator, of course, always be valid so long as the correct flags are passed, I.E LIST_RC_INSTANCE to maintain a reference
	count to the list, and LIST_RC_ITEM to keep a reference count to the item. The former being obvious, but the latter coming extremely
	in handy when you have other threads removing items from the list while iterating (hence, even though when it was retrieved the item
	was valid, it is possible that after, when we relinquish the lock, it gets destroyed). The alternative to this is the internal
	list_for_each function relying on callbacks, which would prevent such things.
*/
#define C_UTILS_LIST_FOR_EACH_REV(item, list) \
	for(C_UTILS_AUTO_ITERATOR _this_iterator = c_utils_list_iterator(list); (item = c_utils_iterator_prev(_this_iterator));)

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_list list_t;
typedef struct c_utils_list_conf list_conf_t;

/*
	Macros
*/
#define LIST_FOR_EACH(...) C_UTILS_LIST_FOR_EACH(__VA_ARGS__)
#define LIST_FOR_EACH_REV(...) C_UTILS_LIST_FOR_EACH_REV(__VA_ARGS__)

/*
	Constants
*/
#define LIST_CONCURRENT C_UTILS_LIST_CONCURRENT
#define LIST_RC_INSTANCE C_UTILS_LIST_RC_INSTANCE
#define LIST_RC_ITEM C_UTILS_LIST_RC_ITEM
#define LIST_DELETE_ON_DESTROY C_UTILS_LIST_DELETE_ON_DESTROY

/*
	Functions
*/
#define list_create(...) c_utils_list_create(__VA_ARGS__)
#define list_create_conf(...) c_utils_list_create_conf(__VA_ARGS__)
#define list_from(...) c_utils_list_from(__VA_ARGS__)
#define list_from_conf(...) c_utils_list_from_conf(__VA_ARGS__)
#define list_get(...) c_utils_list_get(__VA_ARGS__)
#define list_sort(...) c_utils_list_sort(__VA_ARGS__)
#define list_iterator(...) c_utils_list_iterator(__VA_ARGS__)
#define list_add(...) c_utils_list_add(__VA_ARGS__)
#define list_contains(...) c_utils_list_contains(__VA_ARGS__)
#define list_size(...) c_utils_list_size(__VA_ARGS__)
#define list_destroy(...) c_utils_list_destroy(__VA_ARGS__)
#define list_from(...) c_utils_list_from(__VA_ARGS__)
#define list_remove(...) c_utils_list_remove(__VA_ARGS__)
#define list_remove_at(...) c_utils_list_remove_at(__VA_ARGS__)
#define list_remove_all(...) c_utils_list_remove_all(__VA_ARGS__)
#define list_delete(...) c_utils_list_delete(__VA_ARGS__)
#define list_delete_at(...) c_utils_list_delete_at(__VA_ARGS__)
#define list_delete_all(...) c_utils_list_delete_all(__VA_ARGS__)
#define list_as_array(...) c_utils_list_as_array(__VA_ARGS__)
#define list_for_each(...) c_utils_list_for_each(__VA_ARGS__)
#endif

/* End struct c_utils_list function pointers and callbacks. */



/**
 * Allocates and initializes a new empty Linked List.
 * @return Empty c_utils_list or NULL if out of memory error.
 */
struct c_utils_list *c_utils_list_create();

struct c_utils_list *c_utils_list_create_conf(struct c_utils_list_conf *conf);

/**
 * Allocates and initializes a Linked List with the elements from the passed array.
 * Only adds up to the size passed, hence it is vulnerable to overflows if invalid value is passed.
 * If a comparator is passed, it everything will be added in ascending order relative
 * to the comparator used; if the comparator is NULL, it will be added unsorted.
 * @param array Array of elements to be initialized to the linked list.
 * @param size The size of the array; the amount of the array to be read up to.
 * @param compare Used to add sorted, if not NULL, otherwise added unsorted.
 * @return An initialized c_utils_list with all elements, or NULL if out of memory error.
 */
struct c_utils_list *c_utils_list_from(void *array, size_t size);

struct c_utils_list *c_utils_list_from_conf(void *array, size_t size, struct c_utils_list_conf *conf);

/**
 * Retrieves the element at the requested index if in bounds. If it is out of bounds,
 * NULL will be returned instead.
 * @param list The list to retrieve the element from.
 * @param index The index of the element to retrieve.
 * @return The element at the requested index, or NULL if out of bounds or if list is NULL.
 */
void *c_utils_list_get(struct c_utils_list *list, unsigned int index);

/**
 * Removes the item from the list if it is found, along with the node associated with it.
 * If delete_item is NULL, the item removed will not be freed.
 * @param list The list to remove the element from.
 * @param item The item to be searched for.
 * @param delete_item Callback used to free the item.
 * @return 1 on success, 0 if list is NULL, or if the item is not found in the list.
 */
void c_utils_list_remove(struct c_utils_list *list, void *item);

void c_utils_list_delete(struct c_utils_list *list, void *item);

void c_utils_list_delete_at(struct c_utils_list *list, unsigned int index);

/**
 * Removes the item at the given index if it is in bounds. If delete_item is NULL,
 * the item removed will not be freed.
 * @param list The list to remove the element from.
 * @param index The index of the node to be removed from the list.
 * @param delete_item Callback used to free the item.
 * @return The item at the given index, whether or not delete_item is passed, or NULL list is NULL or out of bounds.
 */
void *c_utils_list_remove_at(struct c_utils_list *list, unsigned int index);

/**
 *	Head:
 *		Concurrent:
 *			Yes
 *		Complexity:
 *			O(1)
 *		Notes:
 *			Advances the iterator to the head of the list.
 *	Tail:
 *		Concurrent:
 *			Yes
 *		Complexity:
 *			O(1)
 *		Notes:
 *			Advances the iterator to the tail of the list.
 *	Next:
 *		Concurrent:
 *			Yes
 *		Complexity:
 *			O(1)
 *		Notes:
 *			If the current node is invalidated, it will first check if next is valid, and then if prev is valid.
 *			If next is valid, it will just advance to next, which would have been the next item in the list had
 *			it not been invalidated anyway. If prev is valid, it will jump to prev->next to attempt to bridge
 *			the gap. If neither are valid, the iterator is in an invalidated state and the current position is
 *			reset and it will return a failure.
 *	Prev:
 *		Concurrent:
 *			Yes
 *		Complexity:
 *			O(1)
 *		Notes:
 *			If the current node is invalidated, it will first check if prev is valid, and then if next is valid.
 *			If prev is valid, it will just advance to prev, which would have been the previous item in the list had
 *			it not been invalidated anyway. If next is invalid, it will jump to next->prev to attempt to bridge
 *			the gap. If neither are valid, the iterator is in an invalidated state and the current position is
 *			reset and it will return a failure.
 *	Curr:
 *		Concurrent:
 *			Yes
 *		Complexity:
 *			O(1)
 *		Notes:
 *			Obtains the last item the iterator has iterated over if it is still valid.
 *		
 *
 * @param list Instance of the list.
 * @return A new instance of an iterator, or NULL if list is NULL or if there were problems allocating memory for one. 
 *
 */
struct c_utils_iterator *c_utils_list_iterator(struct c_utils_list *list);

/**
 * Adds the item to the list, in sorted order if the callback is not NULL, or at the tail if it is.
 * @param list List to add the item to.
 * @param item Item to add.
 * @param compare Comparator to add the item sorted.
 * @return 1 upon success, 0 if the list is NULL.
 */
bool c_utils_list_add(struct c_utils_list *list, void *item);

/**
 * Returns an array of items inside of the Linked List, setting the array_size parameter
 * to the size of the array returned.
 * @param list List to convert to an array.
 * @param array_size Used to return the size of the array.
 * @return Array of items, or NULL if list or array_size is NULL.
 */
void *c_utils_list_as_array(struct c_utils_list *list, size_t *array_size);

/**
 * Calls the passed callback on all items in the linked list.
 * @param list List to execute the callback on.
 * @param callback Callback to manipulate the item in the list.
 * @return 1 on success, 0 if list or callback is NULL.
 */
bool c_utils_list_for_each(struct c_utils_list *list, c_utils_general_cb callback);

/**
 * Returns whether or not the list contains the given item.
 * @param list List to search.
 * @param item Item to search for.
 * @return 1 if it does contain the item, 0 if the list is NULL or it doesn't exist in the list.
 */
bool c_utils_list_contains(struct c_utils_list *list, void *item);

void c_utils_list_remove_all(struct c_utils_list *list);

void c_utils_list_delete_all(struct c_utils_list *list);

size_t c_utils_list_size(struct c_utils_list *list);

/**
 * Destroys the linked list, invoking the list's del callback on each item
 * if it is both flagged for (del_items_on_free) and present (del), which are
 * set in the create_conf constructor.
 * @param list List to destroy.
 */
void c_utils_list_destroy(struct c_utils_list *list);

#endif /* C_UTILS_LIST_H */
