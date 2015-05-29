#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdio.h>
#include <stdlib.h>

/* Parameter macros */

/// The default option.
#define NONE 1 << 0
/// Insert before current node. Default is after.
#define BEFORE 1 << 1
/// Delete the current node. Default does not delete, just removes from linked list.
#define DELETE 1 << 2
/// Used to add an item as the first node in the list.
#define FIRST 1 << 3
/// Used to add an item as the last node in the list.
#define LAST 1 << 4
/// Used for insertions, will add it as sorted to the list.
#define SORTED 1 << 5
/// Sorts in descending order. Default is in ascending order.
#define DESCENDING 1 << 6
/// For getting macros passed as parameter.
#define SELECTED(ARGUMENT, MACRO)((ARGUMENT & MACRO))

/* End Parameter macros */

/* Typedef for structs */
typedef struct Node Node;
typedef struct Linked_List Linked_List;
/* End Typedef for structs */

/* Typedef for Linked_List function pointers and callbacks. */


/* Linked_List Insertions */

/// Adds item depending on parameter. FIRST and LAST is obvious, while NONE or BEFORE insert after/before current respectively.
typedef int (*Linked_List_Add)(Linked_List *this, void *item, int parameter);

/* End Linked_List Insertions */


/* Linked_List Removals */

/// Function to remove the item from the list if it finds it.
typedef int (*Linked_List_Remove_Item)(Linked_List *this, void *item, int parameter);
/// Function to remove current node pointed to by current.
typedef int (*Linked_List_Remove_Current)(Linked_List *this, int parameter);
/// Function to remove the item at the specified index;
typedef int (*Linked_List_Remove_At)(Linked_List *list, unsigned int index, int parameter);

/* End Linked_List Removals */


/* Linked_List Retrieval */

/// Function pointer for getting item at the requested index. Basic index checking.
typedef void *(*Linked_List_Get_At)(Linked_List *this, unsigned int index);
/// Function to advance current one position.
typedef void *(*Linked_List_Next)(Linked_List *this);
/// Function to send current back one node.
typedef void *(*Linked_List_Previous)(Linked_List *this);
/// Function to set the current node to the last node in this list.
typedef void *(*Linked_List_Last)(Linked_List *this);
/// Function to set the current node to the first node in this list.
typedef void *(*Linked_List_First)(Linked_List *iterator);

/* End Linked_List Retrieval */


/* Linked_List Callbacks */

/// Callback used on an item to ensure proper deletion.
typedef int (*Linked_List_Delete)(void *item);
/// This should be used on two items to compare the two, for sorting.
typedef int (*Linked_List_Compare)(void *item_one, void *item_two);

/* End Linked_List Callbacks */


/* Linked_List Misc. */

// This should clear all items in the linked list with or without deleting them.
typedef int (*Linked_List_Clear)(Linked_List *this, int parameter);
/// Function to sort the linked list if comparator is set.
typedef int (*Linked_List_Sort)(Linked_List *this, int parameter);

/* End Linked_List Misc. */


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
	Node *first;
	/// The very last node.
	Node *last;
	/// The current node, for iteration purposes. Initialized to first node.
	Node *current;
	/// The current size of the linked list.
	size_t size;
	/// Adds the passed item to the list.
	Linked_List_Add add;
	/// Comparator Callback used for sorting
	Linked_List_Compare compare;
	/// Callback for deleting an item from the list
	Linked_List_Delete delete_item;
	/// Clears the linked list.
	Linked_List_Clear clear;
	/// Gets the item at the specified location. Sets the current node to this.
	Linked_List_Get_At get;
	/// Advances the current node forward one if possible and return the item.
	Linked_List_Next next;
	/// Sends the current node back one if possible, and returns the item.
	Linked_List_Previous prev;
	/// Set current to last node in list.
	Linked_List_Last get_last;
	/// Set current to the first node in list.
	Linked_List_First get_first;
	/// Removes the current node from the list.
	Linked_List_Remove_Current remove_current;
	/// Remove the node at the given index.
	Linked_List_Remove_At remove_at;
	/// Remove the node associated with the passed item if exists.
	Linked_List_Remove_Item remove_item;
	/// Clears the linked list of all items.
	Linked_List_Clear clear;
	/// Sorts the linked list based on comparator.
	Linked_List_Sort sort;
};

/* Create the Linked List with the the supplied callbacks. If NULL is passed for any, the default implementation 
   for the callback will be used in it's place. */
Linked_List *Linked_List_create(Linked_List_Delete delete_item, Linked_List_Compare compare);

/* The standard, default callback used in place of a null Add_Callback parameter. */
int Linked_List_add(Linked_List *this, void *item);

/* The standard, default callback in place of a null Delete_Callback parameter. Just frees the pointer. */
int Linked_List_default_delete(void *item);

/* The standard, default callback in place of a null Delete_Callback parameter. Does nothing and returns 1 (NOP) */
int Linked_List_default_compare(Linked_List *this, void *item_one, void *item_two);

/* Returns the void * at the given index's node. Can remove it from the list entirely. */
void *Linked_List_get_at(Linked_List *this, unsigned int index, int parameter);

/* Sorts the given linked list in either ascending (default) or descending order. Uses Merge Sort. */
int Linked_List_sort(Linked_List *this, int parameter);

/* Removes the Node from the list. */
int Linked_List_remove_node(Linked_List *this, Node *node, int parameter);

/* Removes the item from the list by scanning through all nodes. */
int Linked_List_remove_item(Linked_List *this, void *item, int parameter);

/* Remove the item at the requested index */
int Linked_List_remove_at(Linked_List *this, unsigned int index, int parameter);

/* Returns the next object in the linked list. */
void *Linked_List_next(Linked_List *this);

/* Returns the previous entry in the linked list. */
void * Linked_List_previous(Linked_List *this);

/* Moves the current node to be the last, if it isn't already. */
void * Linked_List_last(Linked_List *this);

/* Moves the current node to be the first, if it isn't already. */
void * Linked_List_first(Linked_List *this);

/* Deletes the current node from the list. */
int Linked_List_remove_current(Linked_List *this, int parameter);

/* Adds the requested item either before or after this. Default is After */
int Linked_List_add(Linked_List *this, void *item, int parameter);

/* Destroys the linked list. Depending on parameter, it can delete all items along with it.*/
void Linked_List_destroy(Linked_List *list, int parameter);


#endif /* LINKED_LIST_H */