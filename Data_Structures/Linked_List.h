#ifndef LINKED_LIST_H
#define LINKED_LIST_H

/*
 * This Linked List implementation is meant for general and generic usage, and is
 * by no means a replacement for more specific-purpose data structures. On top
 * of that, this Linked List should be thread-safe, however iteration is not thread-ideal.
 * 
 * The Linked List allows you to add items in a sorted order and even sort the entire list
 * by means of a passed comparator, and whether or not it chooses to add items in a sorted
 * order depends on whether or not the callback is NULL or not. Hence, if you want
 * to add an item to the list without having to deal with creating and passing a comparator
 * function, you can easily do like so...
 * 
 * Linked_List_add(list, item, NULL);
 * 
 * Inversely, you may add an item in sorted order by doing so like this:
 * 
 * Linked_List_add(list, item, comparator);
 * 
 * Upon deletion, you have the option to delete the item as well as the node by passing
 * a comparator, similar to above.
 * 
 * Linked_List_remove_at(list, 0, NULL);
 * 
 * or
 * 
 * Linked_List_remove_at(list, 0, free);
 * 
 * Iteration is thread-safe, however not thread-ideal. As there is only one iterator,
 * if multiple threads attempt to maneuver it, you may not like the results. Adding,
 * removing and sorting is thread safe and thread ideal, as multiple threads can safely,
 * but in a synchronized way. However, retrieval is is thread-safe and allows multiple
 * threads to read over items in the list, as the Linked List uses a rwlock.
 * 
 * This Linked List, once again is general purpose, and should not be used if performance is of utmost
 * importance. The sort is slow for elements over 2,500, however if you keep it under
 * 1,000, it definitely will do it's job.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

/* Typedef for structs */
typedef struct Node_t Node_t;
typedef struct Linked_List_t Linked_List_t;
/* End Typedef for structs */


/* Linked_List_t Callbacks */

/// Callback used on an item to ensure proper deletion.
typedef void (*Linked_List_Delete)(void *item);
/// This should be used on two items to compare the two, for sorting.
typedef int (*Linked_List_Compare)(void *item_one, void *item_two);

/* End Linked_List_t Callbacks */


/* End Linked_List_t function pointers and callbacks. */

struct Node_t {
    /// Next node in the list.
	Node_t *next;
    /// Previous node in the list.
	Node_t *prev;
    /// Data held by the node.
	void *item;
};

struct Linked_List_t{
	/// The head node of the list.
	Node_t *head;
	/// The tail node of the list.
	Node_t *tail;
	/// The current node for the iterator.
	Node_t *current;
	/// The current size of the linked list.
	size_t size;
	/// Determines whether the list is sorted.
	volatile unsigned char is_sorted;
	/// Ensures only one thread manipulates the items in the list, but multiple threads can read.
	pthread_rwlock_t *manipulating_list;
	/// Ensures that only one thread can move the iterator, but many can read the current value.
	pthread_rwlock_t *manipulating_iterator;
};

/**
 * Allocates and initializes a new empty Linked List.
 * @return Empty Linked_List or NULL if out of memory error.
 */
Linked_List_t *Linked_List_create(void);

/**
 * Allocates and initializes a Linked List with the elements from the passed array.
 * Only adds up to the size passed, hence it is vulnerable to overflows if invalid value is passed.
 * If a comparator is passed, it everything will be added in ascending order relative
 * to the comparator used; if the comparator is NULL, it will be added unsorted.
 * @param array Array of elements to be initialized to the linked list.
 * @param size The size of the array; the amount of the array to be read up to.
 * @param compare Used to add sorted, if not NULL, otherwise added unsorted.
 * @return An initialized Linked_List with all elements, or NULL if out of memory error.
 */
Linked_List_t *Linked_List_create_from(void **array, size_t size, Linked_List_Compare compare);

/**
 * Retrieves the element at the requested index if in bounds. If it is out of bounds,
 * NULL will be returned instead.
 * @param list The list to retrieve the element from.
 * @param index The index of the element to retrieve.
 * @return The element at the requested index, or NULL if out of bounds or if list is NULL.
 */
void *Linked_List_get_at(Linked_List_t *list, unsigned int index);

/**
 * Sort the Linked List relative to the passed comparator. If no comparator is passed,
 * NULL is returned.
 * @param list The list to sort.
 * @param compare The comparator used to sort the list.
 * @return 1 on success, 0 if list or compare is NULL.
 */
bool Linked_List_sort(Linked_List_t *list, Linked_List_Compare compare);

/**
 * Removes the item from the list if it is found, along with the node associated with it.
 * If delete_item is NULL, the item removed will not be freed.
 * @param list The list to remove the element from.
 * @param item The item to be searched for.
 * @param delete_item Callback used to free the item.
 * @return 1 on success, 0 if list is NULL, or if the item is not found in the list.
 */
bool Linked_List_remove_item(Linked_List_t *list, void *item, Linked_List_Delete delete_item);

/**
 * Removes the item at the given index if it is in bounds. If delete_item is NULL, 
 * the item removed will not be freed.
 * @param list The list to remove the element from.
 * @param index The index of the node to be removed from the list.
 * @param delete_item Callback used to free the item.
 * @return The item at the given index, whether or not delete_item is passed, or NULL list is NULL or out of bounds.
 */
void *Linked_List_remove_at(Linked_List_t *list, unsigned int index, Linked_List_Delete delete_item);

/**
 * Advances the Linked List's iterator forward by one if applicable. If the next node
 * is NULL, NULL will be returned instead and the iterator will not be advanced.
 * @param list The list to advance.
 * @return The item at the next node, or NULL if list is NULL or there is no next node in the list.
 */
void *Linked_List_next(Linked_List_t *list);

/**
 * Moves the Linked List's iterator back by one if applicable. If the previous node
 * is NULL, NULL will be returned instead and the iterator will not go back.
 * @param list The list to advance.
 * @return The item at the previous node, or NULL if the list is NULL or there is no previous node.
 */
void * Linked_List_previous(Linked_List_t *list);

/**
 * Moves the Linked List's iterator to the tail of the list if applicable. If the list is
 * empty, NULL will be returned instead and the iterator will not go back.
 * @param list The list to advance.
 * @return The item at the tail node, or NULL if the list is NULL or is empty.
 */
void * Linked_List_tail(Linked_List_t *list);

/**
 * Moves the Linked List's iterator to the head of the list if applicable. If the list is
 * empty, NULL will be returned instead and the iterator will not go back.
 * @param list The list to advance.
 * @return The item at the head node, or NULL if the list is NULL or is empty.
 */
void * Linked_List_head(Linked_List_t *list);

/**
 * Remove the current node from the linked list, calling the callback on the node's item if not NULL.
 * @param list The list to remove the current node from.
 * @param delete_item The callback to call on the node's item.
 * @return The item at the current node, or NULL if the list is empty or the list passed is NULL.
 */
void *Linked_List_remove_current(Linked_List_t *list, Linked_List_Delete delete_item);

/**
 * Adds the requested item after the current node in the list. Note: This flags the
 * list as being unsorted, regardless of whether you take the care to insert it sorted
 * yourself.
 * @param list List to add the item to.
 * @param item Item to be added.
 * @return 1 on success, 0 if the passed list is NULL or if the list is empty.
 */
bool Linked_List_add_after(Linked_List_t *list, void *item);

/**
 * Adds the requested item before the current node in the list. Note: This flags the
 * list as being unsorted, regardless of whether you take the care to insert it sorted
 * yourself.
 * @param list List to add the item to.
 * @param item Item to be added.
 * @return 1 on success, 0 if the passed list is NULL or if the list is empty.
 */
bool Linked_List_add_before(Linked_List_t *list, void *item);

/**
 * Adds the item to the list, in sorted order if the callback is not NULL, or at the tail if it is.
 * @param list List to add the item to.
 * @param item Item to add.
 * @param compare Comparator to add the item sorted.
 * @return 1 upon success, 0 if the list is NULL.
 */
bool Linked_List_add(Linked_List_t *list, void *item, Linked_List_Compare compare);

/**
 * Returns an array of items inside of the Linked List, setting the array_size parameter
 * to the size of the array returned.
 * @param list List to convert to an array.
 * @param array_size Used to return the size of the array.
 * @return Array of items, or NULL if list or array_size is NULL.
 */
void **Linked_List_to_array(Linked_List_t *list, size_t *array_size);

/**
 * Calls the passed callback on all items in the linked list.
 * @param list List to execute the callback on.
 * @param callback Callback to manipulate the item in the list.
 * @return 1 on success, 0 if list or callback is NULL.
 */
bool Linked_List_for_each(Linked_List_t *list, void (*callback)(void *item));

/**
 * Prints all items in a formatted, bracketed and comma separated way based on the
 * to_string callback passed. The items in the list will be represented as such:
 * { item_one, item_two, item_three, ... , item_n } size: n
 * @param list List to print all elements from.
 * @param file The file to print to, I.E stdio or an actual FILE.
 * @param to_string Callback to obtain a string representation of each item in the list.
 */
bool Linked_List_print_all(Linked_List_t *list, FILE *file, char *(*to_string)(void *item));

/**
 * Returns whether or not the list contains the given item.
 * @param list List to search.
 * @param item Item to search for.
 * @return 1 if it does contain the item, 0 if the list is NULL or it doesn't exist in the list.
 */
bool Linked_List_contains(Linked_List_t *list, void *item);

/**
 * Returns the current node's item in the iterator.
 * @param list List to obtain the current element of.
 * @return The current item, or NULL if the list passed is NULL or if the list is empty.
 */
void *Linked_List_get_current(Linked_List_t *list);

/**
 * Destroys the passed linked list, freeing and destroying any of it's members, as well
 * as freeing the list pointer. Note that operations on a linked list should cease before
 * calling this function, as it destroys any and all locks, which may cause deadlocks
 * on threads trying to operate on it. If the delete_item callback is not NULL, or it will be
 * called on every item during deletion, ideally so free can be passed.
 * @param list List to destroy.
 * @param delete_item Callback used on each item.
 */
bool Linked_List_destroy(Linked_List_t *list, Linked_List_Delete delete_item);


#endif /* LINKED_LIST_H */