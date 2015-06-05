#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <pthread.h>
#include <stdio.h>

/* Typedef for structs */
typedef struct Node Node;
typedef struct Linked_List Linked_List;
/* End Typedef for structs */


/* Linked_List Callbacks */

/// Callback used on an item to ensure proper deletion.
typedef void (*Linked_List_Delete)(void *item);
/// This should be used on two items to compare the two, for sorting.
typedef int (*Linked_List_Compare)(void *item_one, void *item_two);

/* End Linked_List Callbacks */


/* End Linked_List function pointers and callbacks. */


/* The struct that holds the next and previous pointers. The type of node,
   A.K.A Node_T, will determine which struct in the union will be used. */
struct Node {
	Node *next;
	Node *prev;
	void *item;
};

/* The Linked List structure which holds the basic information, the first and last node, and
   some callback functions which are to be used to enforce polymorphism. */
struct Linked_List{
	/// The very first node.
	Node *head;
	/// The very last node.
	Node *tail;
	/// The current node, for iteration purposes. Initialized to first node.
	Node *current;
	/// The current size of the linked list.
	size_t size;
	/// Determines whether the list should be sorted before being added in sorted order.
	volatile unsigned char is_sorted;
	/// Ensures only one thread adds or removes items, but multiple threads can read.
	pthread_rwlock_t *adding_or_removing_items;
	/// File for logging information.
	FILE *fp;
};

/// Create a Linked List fully initialized.
Linked_List *Linked_List_create(void);

/// Returns the item at the index. Returns NULL if out of bounds.
void *Linked_List_get_at(Linked_List *this, unsigned int index);

/// Sorts the linked list based on comparator. If comparator is NULL, the list is not sorted.
int Linked_List_sort(Linked_List *this, Linked_List_Compare compare);

/// Remove the item from the linked list, associated with a node in the list. If callback is not NULL, it will be called on the item.
int Linked_List_remove_item(Linked_List *this, void *item, Linked_List_Delete delete_item);

/// Remove the item at the given index, calling the callback on the item the callback is not NULL.
void *Linked_List_remove_at(Linked_List *this, unsigned int index, Linked_List_Delete delete_item);

/// Advances the linked list forward one if applicable, also returning the item at that node.
void *Linked_List_next(Linked_List *this);

/// Moves the linked list back one if applicable, also returning the item at that node.
void * Linked_List_previous(Linked_List *this);

/// Moves the linked list to the very last node in the linked list, returning the item at that node.
void * Linked_List_tail(Linked_List *this);

/// Moves the linked list to the very first node in the linked list, returning the item at that node.
void * Linked_List_head(Linked_List *this);

/// Remove the current node from the linked list, calling the callback on the node's item if not NULL.
void *Linked_List_remove_current(Linked_List *this, Linked_List_Delete delete_item);

/// Adds the item after the current node in the iterator.
int Linked_List_add_after(Linked_List *this, void *item);

/// Adds the item before the current node in the iterator.
int Linked_List_add_before(Linked_List *this, void *item);

/// Adds the item to the linked list, in sorted order if the callback is not null, otherwise at the end.
int Linked_List_add(Linked_List *this, void *item, Linked_List_Compare compare);

/// Destroy the linked list, and if the callback is not null, delete all items as well.
void Linked_List_destroy(Linked_List *list, Linked_List_Delete delete_item);


#endif /* LINKED_LIST_H */