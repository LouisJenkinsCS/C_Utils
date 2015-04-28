#include "Linked_List.h"

Linked_List *Linked_List_create(Linked_List_Delete delete_item, Linked_List_Compare compare){
	Linked_List *list = malloc(sizeof(Linked_List));
	// The first and last nodes are initialized to zero.
	list->first = NULL;
	list->last = NULL;
	// Size initialized to zero.
	list->size = 0;
	// If any of the callbacks are null, the below checks for them and sets their behaviour to an already existing default
	// callback respectively.
	list->delete_item = (delete_item == NULL) ? (&Linked_List_default_delete : delete_item);
	list->compare = (compare == NULL) ? (&Linked_List_default_compare : compare);
	// Set other liste functions here too!
	return list;
}

/* The standard, default callback used in place of a null Add_Callback parameter. */
int Linked_List_add(Linked_List *this, void *item){
	assert(this);
	assert(item);
	// Allocate new node.
	Node *new_node = malloc(sizeof(Node));
	// If malloc returns NULL, then out of memory.
	if(new_node == NULL) return 0;
	// Add the item to the node.
	new_node->item = item;
	new_node->next = NULL;
	// The old last is the new node's previous, as new_node becomes the last node.
	new_node->prev = this->last;
	// Checks if the first and last nodes are NULL, meaning nothing has been inserted yet.
	if(this->first == NULL && this->last == NULL){
		// Sets both first and last nodes as this new_node, as it literally becomes the first and last
		// as no other nodes are present.
		this->first = new_node;
		this->last = new_node;
	} else { // Else, if other nodes exist, then add it after last and bridge the gap.
		// The current last should point to the new node as it's next node.
		this->last->next = new_node;
		// Then set the new last as the new node.
		this->last = new_node;
	}
	return 1; // Returns 1 on success.
}

/* Below are static private functions that can ease the process along without
   being exposed to the user. */

/* Checks if node is found in the linked list. */
static int node_found(Linked_List *list, Node *node){
	Node *temp_node = NULL;
	// Loops until the node is NULL or node is found.
	while((temp_node = list->next)){
		// If node is found, return 1.
		if(node == temp_node) return 1;
	}
	// If node is not found, return 0;
	return 0;
}

/* Removes as if node is only one in list. */
static int remove_only(Linked_List *list, Node *node, int parameter){
	// If the node passed is not in list, return 0.
	if(node_found(list, node)) return 0;
	list->first = NULL;
	list->last = NULL;
	int result = 1;
	if(SELECTED(parameter, DELETE)) result = list->delete_item(&(node->item));
	free(node);
	return result;
}

/* Removes as if node is the first one in the list. */
static int remove_first(Linked_List *list, Node *node, int parameter){
	// If the node is not in the list, return 0.
	if(node_found(list, node)) return 0;
	// Set the new first to be the next in line.
	list->first = list->first->next;
	// Set the new first's previous to NULL.
	list->first->next->prev = NULL;
	int result = 1;
	if(SELECTED(parameter, DELETE)) result = list->delete_item(&(node->item));
	free(node);
	return result;
}

/* Removes as if node is the last one in the list. */
static int remove_last(Linked_List *list, Node *node, int parameter){
	// If the node is not the last in the list, return 0.
	if(list->last != node) return 0;
	// Set the last's previous node as the new last.
	list->last = list->last->prev;
	// Set the new last's next node to NULL.
	list->last->next = NULL;
	// Delete the item node holds.
	int result = 1;
	if(SELECTED(parameter, DELETE)) result = list->delete_item(&(node->item));
	free(node);
	return result;
}

/* Removes as if, as is with the average case, this node is between two other nodes. */
static int remove_normal(Linked_List *list, Node *node, int parameter){
	// If the node is not in the list, return 0.
	if(node_found(list, node) == 0) return 0;
	node->prev->next = node->next;
	node->next->prev = node->prev;
	int result = 1;
	if(SELECTED(parameter, DELETE)) result = list->delete_item(&(node->item));
	free(node);
	return result;
}



/* End of private functions. */


/* The standard, default callback in place of a null Delete_Callback parameter */
int Linked_List_remove_node(Linked_List *this, Node *node, int parameter){
	assert(this);
	assert(node);
	// If the first and last of the linked list is this node, then it's the only node in the list.
	if(this->first == node && this->last == node) return remove_only(this, node, parameter);
	// Else if the last is this node, then it is the last in the list.
	else if(this->last == node) return remove_last(this, node, parameter);
	// Else if the first is this node, then it is the first in the list.
	else if(this->first == node) return remove_first(this, node, parameter);
	// Else, this is a node between first and last and should be treated as such.
	else return remove_normal(this, node, parameter);
	// Should never reach here but just in case.
}


/* The standard, default callback in place of a null delete callback. Just frees the pointer and sets it to null. */
int Linked_List_default_delete(void **item){
	free(*item);
	*item = NULL;
	return 1;
}

/* The standard, default callback in place of a null Delete_Callback parameter. Does nothing and returns 1 (NOP) */
int Linked_List_default_compare(Linked_List *this, void *item_one, void *item_two){
	return 0; // Does nothing, a no-operation.
}

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

/* Destroys the linked list along with all of it's contents. Make sure you get everything from the linked list before
   calling this! */
void Linked_List_destroy(Linked_List *list);