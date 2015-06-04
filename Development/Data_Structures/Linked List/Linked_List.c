#include "Linked_List.h"
#include <Misc_Utils.h>

Linked_List *Linked_List_create(void){
	Linked_List *list = malloc(sizeof(Linked_List));
	if(!list) return 0;
	list->first = list->last = list->current = NULL;
	list->size = 0;
	list->is_sorted = 1;
	list->adding_or_removing_items = malloc(sizeof(pthread_mutex_t));
	if(!adding_or_removing_items){
		free(list);
		return 0;
	}
	list->current_node_change = malloc(sizeof(pthread_mutex_t));
	if(!current_node_change){
		free(list);
		free(adding_or_removing_items);
		return 0;
	}
	pthread_mutex_init(list->adding_or_removing_items, NULL);
	pthread_mutex_init(list->current_node_change, NULL);
	list->fp = fopen("Linked_List_Log.txt", "w");
	return list;
}

int Linked_List_add(Linked_List *this, void *item, Linked_List_Compare compare){
	if(!this) return 0;
	MU_ASSERT_RETURN(item, this->fp, 0);
	Node *new_node; 
	MU_ASSERT_RETURN(new_node = malloc(sizeof(Node)), this->fp, 0);
	new_node->item = item;
	pthread_mutex_lock(this->adding_or_removing_items);
	if(this->first == NULL && this->last == NULL){
		this->first = new_node;
		this->last = new_node;
		this->size++;
		pthread_mutex_unlock(this->adding_or_removing_items);
		return 1;
	}
	int result = 0;
	if(compare) result = add_sorted(this, new_node, compare);
	else result = add_unsorted(this, new_node);
	if(result) this->size++;
	pthread_mutex_unlock(this->adding_or_removing_items);
	return result;
}

/* Below are static private functions that can ease the process along without
   being exposed to the user. */

/* Used to split the array of nodes to be sorted. */
static int split_nodes(Node **array_of_nodes, size_t start, size_t end, Linked_List_Compare comparator){
	// Start initially is 0, End initially is the size of the array.
	assert(array_of_nodes);
	assert(comparator);
	// Continue here.
	return 1;
}

/* Should "merge" the array by treating the first and last index of the two sections of arrays to be merged. */
static int merge_nodes(Node **array_of_nodes, size_t f_Begin, size_t f_End, size_t l_Begin, size_t l_End){
	assert(array_of_nodes);
	// Unfinished!
	return 1;
}

static int add_as_head(Linked_List *this, Node *node){
	node->next = this->head;
	this->head->previous = node;
	this->head = node;
	return 1;
}

static int add_as_tail(Linked_List *this, Node *node){
	this->tail->next = node;
	node->previous = this->tail;
	this->tail = node;
	return 1;
}

static int add_between(Linked_List *this, Node *previous_node, Node *current_node){
	previous_node->previous->next = current_node;
	current_node->next = previous_node;
	current_node->previous = previous_node->previous;
	previous_node->previous = current_node;
	return 1;
}

static int add_sorted(Linked_List *this, Node *node, Linked_List_Compare compare){
	if(!this->is_sorted) Linked_List_sort(this, compare);
	Node *current_node = NULL;
	if(this->size == 1) return compare(node->item, this->head->item) > 0 ? add_as_head(this, node) : add_as_tail(this, node);
	for(current_node = this->head; current_node; current_node = current_node->next){
		if(compare(node->item, current_node->item) > 0) return add_between(this, current_node, node);
	}
	MU_LOG_ERROR(list->fp, "Was unable to add an item, sortedly, to the list!\n");
	return 0;
}

static int add_unsorted(Linked_List *this, Node *node){
	node->next = NULL;
	node->prev = this->last;
	this->last->next = node;
	this->last = node;
	return 1;
}

static int node_exists(Linked_List *list, Node *node){
	assert(list);
	Node *temp_node = NULL;
	for(temp_node = list->first; temp_node; temp_node = temp_node->next) if(temp_node == node) return 1;
	return 0;

}

/* Retrieves the index of the passed node if found. Should be of note that the index here is 1, so the 
   returned index should be decremented first. */
static int node_to_index(Linked_List *list, Node *node){
	int index = 0;
	Node *temp_node = NULL;
	// Loops until the node is NULL or node is found.
	for(temp_node = list->first; temp_node; temp_node = temp_node->next){
		// If node is found, return the index.
		index++;
		if(node == temp_node) return index;
	}
	// If node is not found, return 0;
	return 0;
}

/* Obtains the node that the item is in. Should note, I fixed up the code quite a bit, making it more
   simplistic and shorter, even more understandable and documented. It also no longer uses a parameter to delete the item, 
   instead of just returning an index and removing the item (as clearly this means I have to find the node again anyway via it's index)
   I have it return the node that the item is at instead, allowing the caller to do what they play with the node. */
static Node *item_to_node(Linked_List *list, void *item){
	assert(list);
	Node *node = NULL;
	// Loops until the node is NULL or node is found. If found, return the node.
	for(node = list->first; node ; node = node->next) if(item == node->item) return node;
	// If the node is not found, however, return NULL.
	return NULL;
}

/* Returns the node at that index in the list. It is fixed up and optimized to not check if it is out of bounds,
   if the index is of the last in the list, or if it is the first, which it did not before. Also, it utilizes short circuit
   evaluations to advance in the linked list, safely checking if it is null before dereferencing. There is even an assertion to ensure that
   the list->size is correctly working. */
static Node *index_to_node(Linked_List *list, unsigned int index){
	assert(list);
	// Quick evaluations.
	// If it is out of bounds (remember that the Linked List is zero-based, so if it is equal to the size, it is one index out of bounds.),
	// then return NULL. Saves time.
	if(index >= list->size) return NULL;
	// If the index is clearly the last in the list, just return the last node.
	if(index == list->size - 1) return list->last;
	// If the index is 0, then just return the first.
	if(index == 0) return list->first;
	// Otherwise, scan the list for the node.
	int i = 0;
	Node *node = NULL;
	// This for loop is also a short circuit evaluation. It initialized the node to
	// to the first node in the list. Then, it checks if node is NULL, then if node->next is NULL
	// while also setting node to the next node. Then if the current index is not equal to the passed 
	// index. 
	for(node = list->first; node && node = node->next && i != index; i++);
	// Basically, if the for loop stopped early, it did not then there is something wrong with the program.
	assert(i == index);
	// If it is found, return the node.
	return node;
}

/* Loops through each node, and performs the the passed callback on each item in the list. 
   Return statement should be an integer, so as to add up the amount of successes in proportion to 
   the max size of the array. I.E, if there are 20 nodes, and only 15 of them pass the callback, then
   the result returned will be 15.*/
static int for_each_item(Linked_List *list, int (*callback)(void *item)){
	assert(list);
	assert(callback);
	int result = 0;
	Node *node = list->first;
	if(!node) return 0;
	result += callback(node->item);
	int i = 0;
	// For as long as node is valid and not null, it will call the callback on it,
	// increment by setting node to node->next.
	for(;node ;node = node->next)result += callback(node->item);
	return result;
}

/* Removes as if node is only one in list. */
static int remove_only(Linked_List *list, Node *node, int parameter){
	assert(list);
	list->first = NULL;
	list->last = NULL;
	list->current = NULL;
	if(SELECTED(parameter, DELETE)) list->delete_item(node->item);
	free(node);
	list->size = 0;
	return 1;
}

/* Removes as if node is the first one in the list. */
static int remove_first(Linked_List *list, Node *node, int parameter){
	assert(list);
	// Set the new first to be the next in line.
	list->first = list->first->next;
	// Set the new first's previous to NULL.
	list->first->next->prev = NULL;
	// If the current node is the first, advance it to the next node.
	if(list->current == node) list->current = list->first;
	if(SELECTED(parameter, DELETE)) list->delete_item(node->item);
	free(node);
	list->size--;
	return 1;
}

/* Removes as if node is the last one in the list. */
static int remove_last(Linked_List *list, Node *node, int parameter){
	assert(list);
	// Set the last's previous node as the new last.
	list->last = list->last->prev;
	// Set the new last's next node to NULL.
	list->last->next = NULL;
	// If the current node is the last, then go back one node.
	if(list->current == node) list->current = list->last;
	// Delete the item node holds.
	if(SELECTED(parameter, DELETE)) list->delete_item(node->item);
	// Free the node.
	free(node);
	// Decrement size.
	list->size--;
	return 1;
}

/* Removes as if, as is with the average case, this node is between two other nodes. */
static int remove_normal(Linked_List *list, Node *node, int parameter){
	assert(list);
	node->prev->next = node->next;
	node->next->prev = node->prev;
	if(SELECTED(parameter, DELETE)) list->delete_item(node->item);
	// Advance one node if the ndoe about to be deleted is actually the current one.
	if(list->current == node) list->current = node->next;
	free(node);
	list->size--;
	return 1;
}

/* For each node, it will free each node in the list. */
static int delete_all_nodes(Linked_List *list, int parameter){
	assert(list);
	int result = 0;
	Node *node = NULL;
	int i = 0;
	// For each node, it will call the removal method.
	for(;node = list->first;i++) Linked_List_remove_node(list, node, parameter);
	list->size = 0;
	return 1;
}

/* End of private functions. */


/* The standard, default callback in place of a null Delete_Callback parameter */
int Linked_List_remove_node(Linked_List *this, Node *node, int parameter){
	assert(this);
	assert(node);
	// Check whether or not the node passed exists.
	if(!node_exists(this, node)) return 0;
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
int Linked_List_default_delete(void *item){
	free(item);
	return 1;
}

/* The standard compare function. Literally does nothing worthwhile, it will subtract the two memory addresses, so
   the one declared last in memory will be before the one declared before it. */
int Linked_List_default_compare(Linked_List *this, void *item_one, void *item_two){
	return item_one - item_two; // Memory address subtraction (lol).
}

/* Returns the void * at the given index's node. Can remove it from the list entirely. */
void *Linked_List_get_at(Linked_List *this, unsigned int index){
	assert(this);
	// Get the node at the given index, if it is null, return the item, else NULL.
	Node *temp_node = NULL;
	return temp_node = index_to_node(this, index) ? temp_node->item : NULL;
}

/* Sorts the given linked list in either ascending (default) or descending order. Uses Merge Sort. */
int Linked_List_sort(Linked_List *this, int parameter);

/* Removes the item from the list by scanning through all nodes. */
int Linked_List_remove_item(Linked_List *this, void *item, int parameter){
	assert(this);
	// Passes the passed item to the item_to_node function so it can call the default remove function.
	return Linked_List_remove_node(this, item_to_node(this, item), parameter); 
}

/* Remove the node at the given index. */
int Linked_List_remove_at(Linked_List *this, unsigned int index, int parameter){
	assert(this);
	// Get the node at the given index.
	Node *temp_node = index_to_node(this, index);
	// If temp_node is not null, proceed to remove the node, else return 0.
	return temp_node ? Linked_List_remove_node(this, temp_node, parameter) : 0;
}

int Linked_List_remove_current(Linked_List *this, int parameter){
	assert(this);
	return Linked_List_remove_node(this, this->current, parameter);
}

/* Returns the next object in the iterator. */
void *Linked_List_next(Linked_List *this){
	assert(this);
	// If either the current or the next is null, return NULL.
	if(!this->current || !this->current->next) return NULL;
	// Else, set current to next and return it's item.
	return (this->current = this->current->next)->item;
}

/* Returns the previous entry in the iterator if and only if it is a double linked list. */
void * Linked_List_previous(Linked_List *this){
	assert(this);
	if(!this->current || !this->current->previous) return NULL;
	return (this->current = this->current->previous)->item;
}

/* Moves the current node to be the last, if it isn't already. */
void * Linked_List_last(Linked_List *this){
	assert(this);
	if(!this->last) return NULL;
	return (this->current = this->last)->item;
}

/* Moves the current node to be the first, if it isn't already. */
void * Linked_List_first(Linked_List *this){
	assert(this);
	if(!this->first) return NULL;
	return (this->current = this->first)->item;
}

/* Returns an array of items. */
void **Linked_List_To_Array(Linked_List *this){
	assert(this);
	void **array_of_items = malloc(sizeof(void *) * this->size);
	Node *node = NULL;
	int index = 0;
	for(node = this->first; node; node = node->next){
		array_of_items[index++] = node->item;
	}
	return array_of_items;
}

/* Destroys the linked list along with all of it's contents. Make sure you get everything from the linked list before
   calling this! */
void Linked_List_destroy(Linked_List *list, int parameter){
	assert(list);
	// Free all function pointers and callbacks.
	free(list->remove_at);
	free(list->remove_current);
	free(list->remove_item);
	free(list->sort);
	free(list->next);
	free(list->prev);
	free(list->get_first);
	free(list->get_last);
	free(list->add);
	free(list->delete_item);
	free(list->compare);
	free(list->clear);
	free(list->get);
	// Will delete every node, and also with the possibility of deleting all items.
	delete_all_nodes(list, parameter);
	free(list); // finally, free the list.
}