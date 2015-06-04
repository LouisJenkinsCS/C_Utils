#include "Linked_List.h"
#include <Misc_Utils.h>



/* Begin Static Function Declarations */

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

static int add_as_head(Linked_List *list, Node *node){
	node->next = list->head;
	list->head->previous = node;
	list->head = node;
	return 1;
}

static int add_as_tail(Linked_List *list, Node *node){
	list->tail->next = node;
	node->previous = list->tail;
	list->tail = node;
	return 1;
}

static int add_between(Linked_List *list, Node *previous_node, Node *current_node){
	previous_node->previous->next = current_node;
	current_node->next = previous_node;
	current_node->previous = previous_node->previous;
	previous_node->previous = current_node;
	return 1;
}

static int add_sorted(Linked_List *list, Node *node, Linked_List_Compare compare){
	if(!list->is_sorted) Linked_List_sort(list, compare);
	Node *current_node = NULL;
	if(list->size == 1) return compare(node->item, list->head->item) > 0 ? add_as_head(list, node) : add_as_tail(list, node);
	for(current_node = list->head; current_node; current_node = current_node->next){
		if(compare(node->item, current_node->item) > 0) return add_between(list, current_node, node);
	}
	MU_LOG_ERROR(list->fp, "Was unable to add an item, sortedly, to the list!\n");
	return 0;
}

static int add_unsorted(Linked_List *list, Node *node){
	node->next = NULL;
	node->prev = list->last;
	list->last->next = node;
	list->last = node;
	return 1;
}

static int node_exists(Linked_List *list, Node *node){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	Node *temp_node = NULL;
	for(temp_node = list->first; temp_node; temp_node = temp_node->next) {
		if(temp_node == node) {
			pthread_rwlock_unlock(list->adding_or_removing_items);
			return 1;
		}
	}
	return 0;
}

static int node_to_index(Linked_List *list, Node *node){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	int index = 0;
	Node *temp_node = NULL;
	for(temp_node = list->first; temp_node; temp_node = temp_node->next){
		index++;
		if(node == temp_node) {
			pthread_rwlock_unlock(list->adding_or_removing_items);
			return index;
		}
	}
	pthread_rwlock_unlock(list->adding_or_removing_items);
	MU_LOG_WARNING(list->fp, "Node_To_Index failed as the node was not found!\n");
	return 0;
}

static Node *item_to_node(Linked_List *list, void *item){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	Node *node = NULL;
	for(node = list->first; node ; node = node->next) { 
		if(item == node->item) {
			pthread_rwlock_unlock(list->adding_or_removing_items);
			return node;
		}
	}
	MU_LOG_WARNING(list->fp, "Item_To_Node was unable to find the item in the list, returning NULL");
	pthread_rwlock_unlock(list->adding_or_removing_items);
	return NULL;
}

static Node *index_to_node(Linked_List *list, unsigned int index){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	if(index > list->size) {
		pthread_rwlock_unlock(list->adding_or_removing_items);
		return NULL;
	}
	if(index == list->size) {
		pthread_rwlock_unlock(list->adding_or_removing_items);
		return list->last;
	}
	if(index == 0) {
		pthread_rwlock_unlock(list->adding_or_removing_items);
		return list->first;
	}
	int i = 0;
	Node *node = list->first;
	while(node && node->next && node = node->next && ++i != index);
	pthread_rwlock_unlock(list->adding_or_removing_items);
	MU_ASSERT_RETURN(i == index, list->fp, NULL);
	return node;
}

static int for_each_item(Linked_List *list, void (*callback)(void *item)){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	Node *node = NULL;
	for(node = list->first; node; node = node->next) callback(node->item);
	pthread_rwlock_unlock(list->adding_or_removing_items);
	return 1;
}

static int delete_all_nodes(Linked_List *list, Linked_List_Delete delete_item){
	pthread_rwlock_wrlock(list->adding_or_removing_items);
	while(list->head){
		if(delete_item) delete_item(list->head->item);
		remove_node(list->head);
	}
	pthread_rwlock_unlock(list->adding_or_removing_items);
}

/* Removes as if node is only one in list. */
static int remove_only(Linked_List *list, Node *node, int parameter){
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
	list->first = list->first->next;
	list->first->next->prev = NULL;
	if(list->current == node) list->current = list->first;
	if(SELECTED(parameter, DELETE)) list->delete_item(node->item);
	free(node);
	list->size--;
	return 1;
}

/* Removes as if node is the last one in the list. */
static int remove_last(Linked_List *list, Node *node, int parameter){
	assert(list);
	list->last = list->last->prev;
	list->last->next = NULL;
	if(list->current == node) list->current = list->last;
	if(SELECTED(parameter, DELETE)) list->delete_item(node->item);
	free(node);
	list->size--;
	return 1;
}

/* Removes as if, as is with the average case, this node is between two other nodes. */
static int remove_normal(Linked_List *list, Node *node, int parameter){
	assert(list);
	node->prev->next = node->next;
	node->next->prev = node->prev;
	if(SELECTED(parameter, DELETE)) list->delete_item(node->item);
	if(list->current == node) list->current = node->next;
	free(node);
	list->size--;
	return 1;
}

static int remove_node(Linked_List *list, Node *node, int parameter){
	if(!node_exists(list, node)) {
		MU_LOG_WARNING(list->fp, "Remove_Node failed to find the node in the list!\n");
		return 0;
	}
	if(list->first == node && list->last == node) return remove_only(list, node, parameter);
	else if(list->last == node) return remove_last(list, node, parameter);
	else if(list->first == node) return remove_first(list, node, parameter);
	else return remove_normal(list, node, parameter);
}

/* End of private functions. */


Linked_List *Linked_List_create(void){
	Linked_List *list = malloc(sizeof(Linked_List));
	if(!list) return 0;
	list->first = list->last = list->current = NULL;
	list->size = 0;
	list->is_sorted = 1;
	list->adding_or_removing_items = malloc(sizeof(pthread_rwlock_t));
	if(!adding_or_removing_items){
		free(list);
		return 0;
	}
	pthread_rwlock_init(list->adding_or_removing_items, NULL);
	list->fp = fopen("Linked_List_Log.txt", "w");
	return list;
}

int Linked_List_add(Linked_List *list, void *item, Linked_List_Compare compare){
	if(!list) return 0;
	MU_ASSERT_RETURN(item, list->fp, 0);
	Node *new_node; 
	MU_ASSERT_RETURN(new_node = malloc(sizeof(Node)), list->fp, 0);
	new_node->item = item;
	pthread_rwlock_wrlock(list->adding_or_removing_items);
	if(list->first == NULL && list->last == NULL){
		list->first = new_node;
		list->last = new_node;
		list->size++;
		pthread_rwlock_unlock(list->adding_or_removing_items);
		return 1;
	}
	int result = 0;
	if(compare) result = add_sorted(list, new_node, compare);
	else result = add_unsorted(list, new_node);
	if(result) list->size++;
	pthread_rwlock_unlock(list->adding_or_removing_items);
	return result;
}

void *Linked_List_get_at(Linked_List *list, unsigned int index){
	if(!list) return NULL;
	return (temp_node = index_to_node(list, index)) ? temp_node->item : NULL;
}

int Linked_List_sort(Linked_List *list, Linked_List_Compare compare);

int Linked_List_remove_item(Linked_List *list, void *item, Linked_List_Delete delete_item){
	if(!list) return 0;
	return remove_node(list, item_to_node(list, item), delete_item); 
}

int Linked_List_remove_at(Linked_List *list, unsigned int index, Linked_List_Delete delete_item){
	if(!list) return 0;
	Node *temp_node = index_to_node(list, index);
	return temp_node ? Linked_List_remove_node(list, temp_node, delete_item) : 0;
}

int Linked_List_remove_current(Linked_List *list, Linked_List_Delete delete_item){
	if(!list || !list->current) return 0;
	return remove_node(list, list->current, delete_item);
}

void *Linked_List_next(Linked_List *list){
	if(!list || !list->current || !list->current->next) return NULL;
	return (list->current = list->current->next)->item;
}

void * Linked_List_previous(Linked_List *list){
	if(!list || !list->current || !list->current->previous) return NULL;
	return (list->current = list->current->previous)->item;
}

void * Linked_List_last(Linked_List *list){
	if(!list || !list->last) return NULL;
	return (list->current = list->last)->item;
}

void * Linked_List_first(Linked_List *list){
	if(!list || !list->first) return NULL;
	return (list->current = list->first)->item;
}

void **Linked_List_To_Array(Linked_List *list){
	if(!list) return NULL;
	void **array_of_items = malloc(sizeof(void *) * list->size);
	MU_ASSERT_RETURN(array_of_items, list->fp, NULL);
	Node *node = NULL;
	int index = 0;
	for(node = list->first; node; node = node->next){
		array_of_items[index++] = node->item;
	}
	return array_of_items;
}

void Linked_List_destroy(Linked_List *list, Linked_List_Delete delete_item){
	if(!list) return;
	delete_all_nodes(list, delete_item);
	free(list);
}