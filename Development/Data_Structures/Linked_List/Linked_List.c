#include "Linked_List.h"
#include <Misc_Utils.h>



/* Begin Static Function Declarations */

/* Used to split the array of nodes to be sorted. */
static int split_nodes(Node **array_of_nodes, size_t start, size_t end, Linked_List_Compare comparator){
	return 1;
}

/* Should "merge" the array by treating the head and tail index of the two sections of arrays to be merged. */
static int merge_nodes(Node **array_of_nodes, size_t f_Begin, size_t f_End, size_t l_Begin, size_t l_End){
	return 1;
}

static int add_as_head(Linked_List *list, Node *node){
	node->next = list->head;
	list->head->prev = node;
	list->head = node;
	return 1;
}

static int add_as_tail(Linked_List *list, Node *node){
	list->tail->next = node;
	node->prev = list->tail;
	list->tail = node;
	return 1;
}

static int add_between(Linked_List *list, Node *previous_node, Node *current_node){
	previous_node->prev->next = current_node;
	current_node->next = previous_node;
	current_node->prev = previous_node->prev;
	previous_node->prev = current_node;
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
	node->prev = list->tail;
	list->tail->next = node;
	list->tail = node;
	list->is_sorted = 0;
	return 1;
}

/// Lock before calling this function!
static int node_exists(Linked_List *list, Node *node){
	Node *temp_node = NULL;
	for(temp_node = list->head; temp_node; temp_node = temp_node->next) {
		if(temp_node == node) {
			return 1;
		}
	}
	return 0;
}

/// Obtains lock inside of function!
static int node_to_index(Linked_List *list, Node *node){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	int index = 0;
	Node *temp_node = NULL;
	for(temp_node = list->head; temp_node; temp_node = temp_node->next){
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

/// Obtains lock inside of function!
static Node *item_to_node(Linked_List *list, void *item){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	Node *node = NULL;
	for(node = list->head; node ; node = node->next) { 
		if(item == node->item) {
			pthread_rwlock_unlock(list->adding_or_removing_items);
			return node;
		}
	}
	MU_LOG_WARNING(list->fp, "Item_To_Node was unable to find the item in the list, returning NULL");
	pthread_rwlock_unlock(list->adding_or_removing_items);
	return NULL;
}

/// Obtains lock inside of function!
static Node *index_to_node(Linked_List *list, unsigned int index){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	if(index > list->size) {
		pthread_rwlock_unlock(list->adding_or_removing_items);
		return NULL;
	}
	if(index == list->size) {
		pthread_rwlock_unlock(list->adding_or_removing_items);
		return list->tail;
	}
	if(index == 0) {
		pthread_rwlock_unlock(list->adding_or_removing_items);
		return list->head;
	}
	int i = 0;
	Node *node = list->head;
	while(node && node->next && (node = node->next) && ++i != index);
	pthread_rwlock_unlock(list->adding_or_removing_items);
	MU_ASSERT_RETURN(i == index, list->fp, NULL);
	return node;
}

/// Obtains lock inside of function!
static int for_each_item(Linked_List *list, void (*callback)(void *item)){
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	Node *node = NULL;
	for(node = list->head; node; node = node->next) callback(node->item);
	pthread_rwlock_unlock(list->adding_or_removing_items);
	return 1;
}

/* Removes as if node is only one in list. */
static int remove_only(Linked_List *list, Node *node, Linked_List_Delete delete_item){
	list->head = NULL;
	list->tail = NULL;
	list->current = NULL;
	if(delete_item) delete_item(node->item);
	free(node);
	list->size = 0;
	return 1;
}

/* Removes as if node is the head one in the list. */
static int remove_head(Linked_List *list, Node *node, Linked_List_Delete delete_item){
	list->head = list->head->next;
	list->head->next->prev = NULL;
	if(list->current == node) list->current = list->head;
	if(delete_item) delete_item(node->item);
	free(node);
	list->size--;
	return 1;
}

/* Removes as if node is the tail one in the list. */
static int remove_tail(Linked_List *list, Node *node, Linked_List_Delete delete_item){
	list->tail = list->tail->prev;
	list->tail->next = NULL;
	if(list->current == node) list->current = list->tail;
	if(delete_item) delete_item(node->item);
	free(node);
	list->size--;
	return 1;
}

/* Removes as if, as is with the average case, this node is between two other nodes. */
static int remove_normal(Linked_List *list, Node *node, Linked_List_Delete delete_item){
	node->prev->next = node->next;
	node->next->prev = node->prev;
	if(delete_item) delete_item(node->item);
	if(list->current == node) list->current = node->next;
	free(node);
	list->size--;
	return 1;
}

static int remove_node(Linked_List *list, Node *node, Linked_List_Delete delete_item){
	MU_ASSERT_RETURN(node, list->fp, 0);
	pthread_rwlock_wrlock(list->adding_or_removing_items);
	if(!node_exists(list, node)) {
		MU_LOG_WARNING(list->fp, "Remove_Node failed to find the node in the list!\n");
		return 0;
	}
	int result = 0;
	if(list->head == node && list->tail == node) result = remove_only(list, node, delete_item);
	else if(list->tail == node) result = remove_tail(list, node, delete_item);
	else if(list->head == node) result = remove_head(list, node, delete_item);
	else result = remove_normal(list, node, delete_item);
	pthread_rwlock_unlock(list->adding_or_removing_items);

}

/// Obtains lock inside of function!
static int delete_all_nodes(Linked_List *list, Linked_List_Delete delete_item){
	pthread_rwlock_wrlock(list->adding_or_removing_items);
	while(list->head){
		if(delete_item) delete_item(list->head->item);
		remove_node(list, list->head, delete_item);
	}
	pthread_rwlock_unlock(list->adding_or_removing_items);
}

/* End of private functions. */


Linked_List *Linked_List_create(void){
	Linked_List *list = malloc(sizeof(Linked_List));
	if(!list) return 0;
	list->head = list->tail = list->current = NULL;
	list->size = 0;
	list->is_sorted = 1;
	list->adding_or_removing_items = malloc(sizeof(pthread_rwlock_t));
	if(!list->adding_or_removing_items){
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
	if(list->head == NULL && list->tail == NULL){
		list->head = new_node;
		list->tail = new_node;
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
	Node *node = NULL;
	return (node = index_to_node(list, index)) ? node->item : NULL;
}

int Linked_List_sort(Linked_List *list, Linked_List_Compare compare){
	return 0;
}

int Linked_List_remove_item(Linked_List *list, void *item, Linked_List_Delete delete_item){
	if(!list) return 0;
	return remove_node(list, item_to_node(list, item), delete_item); 
}

int Linked_List_remove_at(Linked_List *list, unsigned int index, Linked_List_Delete delete_item){
	if(!list) return 0;
	Node *temp_node = index_to_node(list, index);
	return temp_node ? remove_node(list, temp_node, delete_item) : 0;
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
	if(!list || !list->current || !list->current->prev) return NULL;
	return (list->current = list->current->prev)->item;
}

void * Linked_List_tail(Linked_List *list){
	if(!list || !list->tail) return NULL;
	return (list->current = list->tail)->item;
}

void * Linked_List_head(Linked_List *list){
	if(!list || !list->head) return NULL;
	return (list->current = list->head)->item;
}

void **Linked_List_To_Array(Linked_List *list){
	if(!list) return NULL;
	void **array_of_items = malloc(sizeof(void *) * list->size);
	MU_ASSERT_RETURN(array_of_items, list->fp, NULL);
	Node *node = NULL;
	pthread_rwlock_rdlock(list->adding_or_removing_items);
	int index = 0;
	for(node = list->head; node; node = node->next){
		array_of_items[index++] = node->item;
	}
	pthread_rwlock_unlock(list->adding_or_removing_items);
	return array_of_items;
}

void Linked_List_destroy(Linked_List *list, Linked_List_Delete delete_item){
	if(!list) return;
	delete_all_nodes(list, delete_item);
	pthread_rwlock_destroy(list->adding_or_removing_items);
	free(list->adding_or_removing_items);
	fclose(list->fp);
	free(list);
}