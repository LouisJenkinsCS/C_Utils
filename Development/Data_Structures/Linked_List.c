#include "Linked_List.h"

Linked_List *Linked_List_create(Linked_List_Add_Callback add, Linked_List_Delete_Callback del, Linked_List_Compare_Callback cmp, Node_T node_type){
	Linked_List *list = malloc(sizeof(Linked_List));
	// The first and last nodes are initialized to zero.
	list->first = NULL;
	list->last = NULL;
	list->size = 0;
	list->node_type = node_type;
	// If any of the callbacks are null, the below checks for them and sets their behaviour to an already existing default
	// callback respectively.
	list->Linked_List_Add_Callback = Linked_List_Add_Callback == NULL ? &Linked_List_default_add : Linked_List_Add_Callback;
	list->Linked_List_Delete_Callback = Linked_List_Delete_Callback == NULL ? &Linked_List_default_delete : Linked_List_Delete_Callback;
	list->Linked_List_Compare_Callback = Linked_List_Compare_Callback == NULL ? &Linked_List_default_compare : Linked_List_Compare_Callback;
	return list;
}

/* The standard, default callback used in place of a null Add_Callback parameter. */
int Linked_List_default_add(Linked_List *this, void *item){
	assert(this);
	assert(item);
	Node *new_node = malloc(sizeof(Node));
	// Add the item to the node.
	new_node->item = item;
	// This assigns the proper union based on the Linked List's node_type.
	switch(this->node_type){
		case SINGLE:
			new_node->single_next = NULL;
			break;
		case DOUBLE:
			new_node->double_next = NULL;
			// The old last is the new node's previous, as it becomes the new last node.
			new_node->double_prev = this->last;
	}
	// Checks if the first and last nodes are NULL, meaning nothing has been inserted yet.
	if(this->first == NULL && this->last == NULL){
		// Sets both first and last nodes as this new_node, as it literally becomes the first and last
		// as no other nodes are present.
		this->first = new_node;
		this->last = new_node;
	} else { // Else, if other nodes exist, then add it after last and bridge the gap.
		if(this->node_type == SINGLE){ // If SINGLE linked list...
			// Place this node at the end of the linked list.
			this->last->single_next = new_node;
			// Set this node as the current last.
			this->last = new_node;
		} else { // Else, if a DOUBLE linked list.
			// Like single, place the node at the end of this linked list.
			this->last->double_next = new_node; 
			// Also place the current last as new_node's previous.
			this->last = new_node;
		}
	}
	return 1; // Returns 1 on success.
}

/* The standard, default callback in place of a null Delete_Callback parameter */
int Linked_List_default_delete(Linked_List *this, Node *node){
	assert(this);
	assert(node);
	if(this->node_type == SINGLE){
		if(node->single_next == NULL){ // If this node is the head of the list...
			// Check if the first of the list is this node...
			// If so, then just remove it.
			// Else, then remove it, iterate through the list, and set the one before this one to be the List's Last.
		} else { // Else if this node is not the head of the list.
			// Continue here!
		}
	}
	return 1;
}

/* The standard, default callback in place of a null Delete_Callback parameter */
int Linked_List_default_compare(Linked_List *this, void *item_one, void *item_two);

/* Returns an iterator for the linked list. This function will also set the required callbacks as well. */
Iterator *Linked_List_get_iterator(Linked_List *this);

/* Returns the next object in the iterator. */
void * Iterator_get_next(Iterator *iterator);

/* Returns the previous entry in the iterator if and only if it is a double linked list. */
void * Iterator_get_previous(Iterator *iterator);

/* A no-operation function that gets set on the previous callback if it is a single linked list. Returns NULL. */
void * Iterator_nop(Iterator *iterator);

/* Moves the current node to be the last, if it isn't already. */
void * Iterator_get_last(Iterator *iterator);

/* Moves the current node to be the first, if it isn't already. */
void * Iterator_get_first(Iterator *iterator);

/* Deletes the current node from the list. */
int Iterator_delete_current(Iterator *iterator);

/* Appends the item to the current node (as a node) */
int Iterator_append_current(Iterator *iterator, void *item);

/* Prepends the item to the current node (as a node) */
int Iterator_prepend_current(Iterator *iterator, void *item);

/* Deletes this iterator. Does not destroy the linked list along with it. */
void Iterator_destroy(Iterator *iterator);

/* Destroys the linked list along with all of it's contents. Make sure you get everything from the linked list before
   calling this! */
void Linked_List_destroy(Linked_List *list);
