#ifndef DS_LIST_H
#define DS_LIST_H

#include <pthread.h>
#include <DS_Iterator.h>
#include <stdbool.h>
#include <stdio.h>
#include <DS_Helpers.h>

/* End DS_List_t function pointers and callbacks. */

typedef struct {
	/// The head node of the list.
	DS_Node_t *head;
	/// The tail node of the list.
	DS_Node_t *tail;
	/// The current size of the linked list.
	volatile size_t size;
	/// Determines whether the list is sorted.
	volatile unsigned char is_sorted;
	/// Ensures only one thread manipulates the items in the list, but multiple threads can read.
	pthread_rwlock_t *rwlock;
} DS_List_t;

/**
 * Allocates and initializes a new empty Linked List.
 * @return Empty DS_List or NULL if out of memory error.
 */
DS_List_t *DS_List_create(bool synchronized);

/**
 * Allocates and initializes a Linked List with the elements from the passed array.
 * Only adds up to the size passed, hence it is vulnerable to overflows if invalid value is passed.
 * If a comparator is passed, it everything will be added in ascending order relative
 * to the comparator used; if the comparator is NULL, it will be added unsorted.
 * @param array Array of elements to be initialized to the linked list.
 * @param size The size of the array; the amount of the array to be read up to.
 * @param compare Used to add sorted, if not NULL, otherwise added unsorted.
 * @return An initialized DS_List with all elements, or NULL if out of memory error.
 */
DS_List_t *DS_List_create_from(void **array, size_t size, DS_comparator_cb compare, bool synchronized);

/**
 * Retrieves the element at the requested index if in bounds. If it is out of bounds,
 * NULL will be returned instead.
 * @param list The list to retrieve the element from.
 * @param index The index of the element to retrieve.
 * @return The element at the requested index, or NULL if out of bounds or if list is NULL.
 */
void *DS_List_get(DS_List_t *list, unsigned int index);

/**
 * Sort the Linked List relative to the passed comparator. If no comparator is passed,
 * NULL is returned.
 * @param list The list to sort.
 * @param compare The comparator used to sort the list.
 * @return 1 on success, 0 if list or compare is NULL.
 */
bool DS_List_sort(DS_List_t *list, DS_comparator_cb compare);

/**
 * Removes the item from the list if it is found, along with the node associated with it.
 * If delete_item is NULL, the item removed will not be freed.
 * @param list The list to remove the element from.
 * @param item The item to be searched for.
 * @param delete_item Callback used to free the item.
 * @return 1 on success, 0 if list is NULL, or if the item is not found in the list.
 */
bool DS_List_remove_item(DS_List_t *list, void *item, DS_delete_cb delete_item);

/**
 * Removes the item at the given index if it is in bounds. If delete_item is NULL, 
 * the item removed will not be freed.
 * @param list The list to remove the element from.
 * @param index The index of the node to be removed from the list.
 * @param delete_item Callback used to free the item.
 * @return The item at the given index, whether or not delete_item is passed, or NULL list is NULL or out of bounds.
 */
void *DS_List_remove_at(DS_List_t *list, unsigned int index, DS_delete_cb delete_item);

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
DS_Iterator_t *DS_List_iterator(DS_List_t *list);

/**
 * Adds the item to the list, in sorted order if the callback is not NULL, or at the tail if it is.
 * @param list List to add the item to.
 * @param item Item to add.
 * @param compare Comparator to add the item sorted.
 * @return 1 upon success, 0 if the list is NULL.
 */
bool DS_List_add(DS_List_t *list, void *item, DS_comparator_cb compare);

/**
 * Returns an array of items inside of the Linked List, setting the array_size parameter
 * to the size of the array returned.
 * @param list List to convert to an array.
 * @param array_size Used to return the size of the array.
 * @return Array of items, or NULL if list or array_size is NULL.
 */
void **DS_List_to_array(DS_List_t *list, size_t *array_size);

/**
 * Calls the passed callback on all items in the linked list.
 * @param list List to execute the callback on.
 * @param callback Callback to manipulate the item in the list.
 * @return 1 on success, 0 if list or callback is NULL.
 */
bool DS_List_for_each(DS_List_t *list, DS_general_cb callback);

/**
 * Prints all items in a formatted, bracketed and comma separated way based on the
 * to_string callback passed. The items in the list will be represented as such:
 * { item_one, item_two, item_three, ... , item_n } size: n
 * @param list List to print all elements from.
 * @param file The file to print to, I.E stdio or an actual FILE.
 * @param to_string Callback to obtain a string representation of each item in the list.
 */
bool DS_List_print_all(DS_List_t *list, FILE *file, DS_to_string_cb to_string);

/**
 * Returns whether or not the list contains the given item.
 * @param list List to search.
 * @param item Item to search for.
 * @return 1 if it does contain the item, 0 if the list is NULL or it doesn't exist in the list.
 */
bool DS_List_contains(DS_List_t *list, void *item);

bool DS_List_clear(DS_List_t *list, DS_delete_cb del);

/**
 * Destroys the passed linked list, freeing and destroying any of it's members, as well
 * as freeing the list pointer. Note that operations on a linked list should cease before
 * calling this function, as it destroys any and all locks, which may cause deadlocks
 * on threads trying to operate on it. If the delete_item callback is not NULL, or it will be
 * called on every item during deletion, ideally so free can be passed.
 * @param list List to destroy.
 * @param delete_item Callback used on each item.
 */
bool DS_List_destroy(DS_List_t *list, DS_delete_cb del);


#endif /* DS_LIST_H */