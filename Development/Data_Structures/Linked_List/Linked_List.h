#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <pthread.h>
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
int Linked_List_sort(Linked_List_t *list, Linked_List_Compare compare);

/**
 * Removes the item from the list if it is found, along with the node associated with it.
 * If delete_item is NULL, the item removed will not be freed.
 * @param list The list to remove the element from.
 * @param item The item to be searched for.
 * @param delete_item Callback used to free the item.
 * @return 1 on success, 0 if list is NULL, or if the item is not found in the list.
 */
int Linked_List_remove_item(Linked_List_t *list, void *item, Linked_List_Delete delete_item);

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

/// Moves the linked list to the very last node in the linked list, returning the item at that node.
void * Linked_List_tail(Linked_List_t *list);

/// Moves the linked list to the very first node in the linked list, returning the item at that node.
void * Linked_List_head(Linked_List_t *list);

/// Remove the current node from the linked list, calling the callback on the node's item if not NULL.
void *Linked_List_remove_current(Linked_List_t *list, Linked_List_Delete delete_item);

/// Adds the item after the current node in the iterator.
int Linked_List_add_after(Linked_List_t *list, void *item);

/// Adds the item before the current node in the iterator.
int Linked_List_add_before(Linked_List_t *list, void *item);

/// Adds the item to the linked list, in sorted order if the callback is not null, otherwise at the end.
int Linked_List_add(Linked_List_t *list, void *item, Linked_List_Compare compare);

/// Returns an array of items in the list, setting the size parameter to the size of the array.
void **Linked_List_to_array(Linked_List_t *list, size_t *array_size);

/// Calls the callback on each item in the list.
int Linked_List_for_each(Linked_List_t *list, void (*callback)(void *item));

/// Prints all items in the Linked List like such: { ... }
void Linked_List_print_all(Linked_List_t *list, FILE *file, char *(*to_string)(void *item));

/// Returns whether or not the item exists in the Linked_List
int Linked_List_contains(Linked_List_t *list, void *item);

/// Returns the current item being iterated over.
void *Linked_List_get_current(Linked_List_t *list);

/// Destroy the linked list, and if the callback is not null, delete all items as well.
void Linked_List_destroy(Linked_List_t *list, Linked_List_Delete delete_item);


#endif /* LINKED_LIST_H */