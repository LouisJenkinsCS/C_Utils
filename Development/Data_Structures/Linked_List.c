#include "Linked_List.h"

Linked_List *Linked_List_create(Add_Callback add, Delete_Callback delete, Compare_Callback compare, Node_T node_type){
	Linked_List *list = malloc(sizeof(Linked_List));
	// The first and last nodes are initialized to zero.
	list->first = NULL;
	list->last = NULL;
	list->size = 0;
	list->node_type = node_type;
	// If any of the callbacks are null, the below checks for them and sets their behaviour to an already existing default
	// callback respectively.
	list->add = (add == NULL) ? (&Linked_List_default_add : add);
	list->delete = (delete == NULL) ? (&Linked_List_default_delete : delete);
	list->compare = (compare == NULL) ? (&Linked_List_default_compare : compare);
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

/* Below are static private functions that can ease the process along without
   being exposed to the user. */

#define type_of_node(type, function, arg1, arg2)(type ##_ ## function (arg1, arg2)) // not sure if this would work?

/* Removes the node as if it is the only node in the linked list. */
static int single_remove_only(Linked_List *list, Node *node){
	// list->first = NULL;
	// list->last = NULL;
	// list->delete(node->item);
	// free(node);
	// return 1 on success.
}

/* Removes the node as if it is the first node in the linked list. */
static int single_remove_first(Linked_List *list, Node *node){
	// list->first = node->next;
	// list->delete(node->item);
	// free(node);
	// return 1 on success.
}

/* Removes the node as if it is the last node in the linked list. */
static int single_remove_last(Linked_List *list, Node *node){
	// Iterate through list to get node before this node. Assign to var previous_node.
	// list->last = previous_node.
	// previous_node->next = NULL.
	// list->delete(node->item);
	// free(node);
	// return 1 on success.
}

/* Remove the node as if it were normal (I.E Has a node before and after it, is not head/first or tail/last) */
static int single_remove_normal(Linked_List *list, Node *node){
	// Iterate through list to get node before this node. Assign to var previous_node.
	// previous_node->next = node->next;
	// list->delete(node->item);
	// free(node);
	// return 1 on success.
}

static int double_remove_first(Linked_List *list, Node *node);

static int double_remove_last(Linked_List *list, Node *node);

static int double_remove_normal(Linked_List *list, Node *node);

static int tree_remove_root(Linked_List *list, Node *node);

static int tree_remove_leaf(Linked_List *list, Node *node);

/* End of private functions. */


/* The standard, default callback in place of a null Delete_Callback parameter */
int Linked_List_default_delete(Linked_List *this, Node *node){
	assert(this);
	assert(node);
	// If the first and last of the linked list is this node, then it's the only node in the list.
	if(this->first == node && this->last == node) return type_of_node((this->node_type == SINGLE) ? (SINGLE : this->node_type == DOUBLE) ? (DOUBLE, TREE),
		 remove_only, this, node);
	// Else if the last is this node, then it is the last in the list.
	else if(this->last == node) return type_of_node((this->node_type == SINGLE) ? (SINGLE : this->node_type == DOUBLE) ? (DOUBLE, TREE),
		 remove_last, this, node);
	// Else if the first is this node, then it is the first in the list.
	else if(this->first == node) return type_of_node(
		(this->node_type == SINGLE) ? (SINGLE : this->node_type == DOUBLE) ? (DOUBLE, TREE),
		 remove_first, this, node);
	// Else, this is a node between first and last and should be treated as such.
	else return type_of_node((this->node_type == SINGLE) ? (SINGLE : this->node_type == DOUBLE) ? (DOUBLE, TREE),
		 remove_normal, this, node);
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

/* Add before or after current node depending on the parameter */
int Iterator_add(Iterator *iterator, int parameter);

/* Deletes this iterator. Does not destroy the linked list along with it. */
void Iterator_destroy(Iterator *iterator);

/* Destroys the linked list along with all of it's contents. Make sure you get everything from the linked list before
   calling this! */
void Linked_List_destroy(Linked_List *list);
