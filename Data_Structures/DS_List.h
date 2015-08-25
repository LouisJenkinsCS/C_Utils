#ifndef DS_LIST_H
#define DS_LIST_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <DS_Helpers.h>

/* End DS_List_t function pointers and callbacks. */

typedef struct {
	/// The head node of the list.
	DS_Node_t *head;
	/// The tail node of the list.
	DS_Node_t *tail;
	/// The current node for the iterator.
	DS_Node_t *current;
	/// The current size of the linked list.
	size_t size;
	/// Determines whether the list is sorted.
	volatile unsigned char is_sorted;
	/// Ensures only one thread manipulates the items in the list, but multiple threads can read.
	pthread_rwlock_t *manipulating_list;
	/// Ensures that only one thread can move the iterator, but many can read the current value.
	pthread_rwlock_t *manipulating_iterator;
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
void *DS_List_get_at(DS_List_t *list, unsigned int index);

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
 * Advances the Linked List's iterator forward by one if applicable. If the next node
 * is NULL, NULL will be returned instead and the iterator will not be advanced.
 * @param list The list to advance.
 * @return The item at the next node, or NULL if list is NULL or there is no next node in the list.
 */
void *DS_List_next(DS_List_t *list);

/**
 * Moves the Linked List's iterator back by one if applicable. If the previous node
 * is NULL, NULL will be returned instead and the iterator will not go back.
 * @param list The list to advance.
 * @return The item at the previous node, or NULL if the list is NULL or there is no previous node.
 */
void * DS_List_previous(DS_List_t *list);

/**
 * Moves the Linked List's iterator to the tail of the list if applicable. If the list is
 * empty, NULL will be returned instead and the iterator will not go back.
 * @param list The list to advance.
 * @return The item at the tail node, or NULL if the list is NULL or is empty.
 */
void * DS_List_tail(DS_List_t *list);

/**
 * Moves the Linked List's iterator to the head of the list if applicable. If the list is
 * empty, NULL will be returned instead and the iterator will not go back.
 * @param list The list to advance.
 * @return The item at the head node, or NULL if the list is NULL or is empty.
 */
void * DS_List_head(DS_List_t *list);

/**
 * Remove the current node from the linked list, calling the callback on the node's item if not NULL.
 * @param list The list to remove the current node from.
 * @param delete_item The callback to call on the node's item.
 * @return The item at the current node, or NULL if the list is empty or the list passed is NULL.
 */
void *DS_List_remove_current(DS_List_t *list, DS_delete_cb delete_item);

/**
 * Adds the requested item after the current node in the list. Note: This flags the
 * list as being unsorted, regardless of whether you take the care to insert it sorted
 * yourself.
 * @param list List to add the item to.
 * @param item Item to be added.
 * @return 1 on success, 0 if the passed list is NULL or if the list is empty.
 */
bool DS_List_add_after(DS_List_t *list, void *item);

/**
 * Adds the requested item before the current node in the list. Note: This flags the
 * list as being unsorted, regardless of whether you take the care to insert it sorted
 * yourself.
 * @param list List to add the item to.
 * @param item Item to be added.
 * @return 1 on success, 0 if the passed list is NULL or if the list is empty.
 */
bool DS_List_add_before(DS_List_t *list, void *item);

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

/**
 * Returns the current node's item in the iterator.
 * @param list List to obtain the current element of.
 * @return The current item, or NULL if the list passed is NULL or if the list is empty.
 */
void *DS_List_get_current(DS_List_t *list);

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