#ifndef C_UTILS_LIST_H
#define C_UTILS_LIST_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "iterator.h"
#include "helpers.h"
#include "../io/logger.h"

struct c_utils_list;

struct c_utils_list_conf {
	/// Determines if reader-writer locks are used for concurrent access.
	bool concurrent;
	/// Determines if this list should be reference counted.
	bool ref_counted;
	/// Determines if the list's items should be del'd on destroy
	bool del_items_on_free;
	/// Used to sort the list of items when mutated.
	c_utils_comparator_cb cmp;
	/// Used to delete the item in the list. Defaults to free()
	c_utils_delete_cb del;
	/// Used to log any errors or trace information to.
	struct c_utils_logger *logger;
};

#define C_UTILS_LIST_FOR_EACH(tmp_var, list) for(C_UTILS_AUTO_ITERATOR auto_it = c_utils_list_iterator(list); (tmp_var = c_utils_iterator_next(auto_it));)

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
 * Creates and initializes an iterator for the linked list. The following operations supported are below:
 * Next, Prev, Append, Prepend, Head, Tail.
 *
 * The list also features a node-correction algorithm, which if the current node has already been removed from the list, it will
 * attempt to start at the next or previous node instead of the starting at the beginning if possible. The iterator must be freed
 * no longer in use, and the iterator does not get updated if the list instance it belongs to gets freed, so use with caution.
 *
 * All iterator functions use the list's internal read-write lock, so it is not a lock-less operation, but it will not block for a 
 * prolonged period of time. Hence, due to the usage of a read-write lock, it allows concurrent threads to iterate over the list so long
 * as they do not attempt to append or prepend to the list. 
 *
 * Finally, this iterator is barebones and is a work-in-progress, so use with caution
 *
 * @param list Instance of the list.
 * @return A new instance of an iterator, or NULL if list is NULL or if there were problems allocating memory for one. 
 *
 */
struct c_utils_iterator *c_utils_list_iterator(struct c_utils_list *list);

bool c_utils_list_sort(struct c_utils_list *list);

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
void **c_utils_list_as_array(struct c_utils_list *list, size_t *array_size);

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

bool c_utils_list_clear(struct c_utils_list *list);

void c_utils_list_remove_all(struct c_utils_list *list);

void c_utils_list_delete_all(struct c_utils_list *list);

size_t c_utils_list_size(struct c_utils_list *list);

/**
 * Destroys the passed linked list, freeing and destroying any of it's members, as well
 * as freeing the list pointer. Note that operations on a linked list should cease before
 * calling this function, as it destroys any and all locks, which may cause deadlocks
 * on threads trying to operate on it. If the delete_item callback is not NULL, or it will be
 * called on every item during deletion, ideally so free can be passed.
 * @param list List to destroy.
 * @param delete_item Callback used on each item.
 */
void c_utils_list_destroy(struct c_utils_list *list);

#endif /* C_UTILS_LIST_H */
