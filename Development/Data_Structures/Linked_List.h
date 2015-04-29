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
/// Removes the item from the list after retrieving it. Default is to keep it in the list.
#define REMOVE 1 << 3
/// Sorts in descending order. Default is in ascending order.
#define DESCENDING 1 << 4
/// For getting macros passed as parameter.
#define SELECTED(ARGUMENT, MACRO)((ARGUMENT & MACRO))

/* Typedef for the structs */
typedef struct Node Node;
typedef struct Linked_List Linked_List;
typedef struct Iterator Iterator;

/* Typedef for Linked_List function pointers */
/// This one should be callback used on an item to delete it.
typedef int (*Linked_List_Delete)(void *item);
/// This should be used on two items to compare the two, for sorting.
typedef int (*Linked_List_Compare)(void *item_one, void *item_two);
/// Function pointer for remove...
typedef int (*Linked_List_Remove)(Linked_List *this, Node *node);
// This should clear all items in the linked list with or without deleting them.
typedef int (*Linked_List_Clear)(Linked_List *this, int parameter);
/// Function pointer for adding...
typedef int (*Linked_List_Add)(Linked_List *this, Node *node);
/// Function pointer for getting item at the requested index. Basic index checking.
typedef void *(*Linked_List_Get_At)(Linked_List *this, unsigned int index);
/// Function to receive an iterator for this linked list.
typedef Iterator *(Linked_List_Get_Iterator)(Linked_List *this);
/// Function to sort the linked list if comparator is set.
typedef int (*Linked_List_Sort)(Linked_List *this, int parameter);

/* Typedef for Iterator function pointers */
/// Gets the next item... unfortunately, can't forward the node to the next.
/// #define Iterator_Get_Current(this, type)((type *)((this)->(current)->item))

typedef void *(*Iterator_Next)(Iterator *iterator);
typedef void *(*Iterator_Previous)(Iterator *iterator);
typedef void *(*Iterator_Last)(Iterator *iterator);
typedef void *(*Iterator_First)(Iterator *iterator);

typedef int (*Iterator_Remove)(Iterator *iterator);
typedef int (*Iterator_Add)(Iterator *iterator, void *item, int parameter);

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
	/// The current size of the linked list.
	size_t size;
	/// Removes an item from the list.
	Linked_List_Remove remove;
	/// Comparator Callback used for sorting
	Linked_List_Compare compare;
	/// Callback for deleting an item from the list
	Linked_List_Delete delete_item;
	/// Clears the linked list.
	Linked_List_Clear clear;
	/// Gets the item at the specified location.
	Linked_List_Get_At get;
	/// Gets an iterator for this instance
	Linked_List_Get_Iterator get_iterator;
};

/* A barebones iterator */
struct Iterator {
	/// The linked list to iterate through.
	Linked_List *list;
	/// The current node in the list the iterator is on.
	Node *current;
	/// Gets the next node in the linked list.
	Iterator_Next next;
	/// Sets Iterator back one.
	Iterator_Previous prev;
	/// Function pointer to set current to the very last node.
	Iterator_Last last;
	/// Function pointer to set current to the very first in the linked list.
	Iterator_First first;
	/// Function pointer to remove the current item from the list.
	Iterator_Remove remove;
	/// Function pointer to add item to the linked list, before or after.
	Iterator_Add add;
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

/* Returns an iterator for the linked list. This function will also set the required callbacks as well. */
Iterator *Linked_List_get_iterator(Linked_List *this);

/* Returns the void * at the given index's node. Can remove it from the list entirely. */
void *Linked_List_get_at(Linked_List *this, unsigned int index, int parameter);

/* Sorts the given linked list in either ascending (default) or descending order. Uses Merge Sort. */
int Linked_List_sort(Linked_List *this, int parameter);

/* Removes the Node from the list. */
int Linked_List_remove_node(Linked_List *this, Node *node, int parameter);

/* Removes the item from the list by scanning through all nodes. */
int Linked_List_remove_item(Linked_List *this, void *item, int parameter);

int Linked_List_remove_at(Linked_List *this, unsigned int index, int parameter);

/* Returns the next object in the iterator. */
void *Iterator_next(Iterator *iterator);

/* Returns the previous entry in the iterator if and only if it is a double linked list. */
void * Iterator_previous(Iterator *iterator);

/* Moves the current node to be the last, if it isn't already. */
void * Iterator_last(Iterator *iterator);

/* Moves the current node to be the first, if it isn't already. */
void * Iterator_first(Iterator *iterator);

/* Deletes the current node from the list. */
int Iterator_remove(Iterator *iterator);

/* Adds the requested item either before or after this. Default is After */
int Iterator_add(Iterator *iterator, void *item, int parameter);

/* Deletes this iterator. Does not destroy the linked list along with it. */
void Iterator_destroy(Iterator *iterator);

/* Destroys the linked list. Depending on parameter, it can delete all items along with it.*/
void Linked_List_destroy(Linked_List *list, int parameter);










#endif /* LINKED_LIST_H */