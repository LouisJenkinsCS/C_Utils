#include "Linked_List.h"
#include <Misc_Utils.h>

/// Static logger for all linked lists do use.
static MU_Logger_t *logger = NULL;

/* Below are declarations for a private sub-list for the merge sort algorithm, which may not be necessary, but it certainly
   makes it easier overall to visualize how to implement this. The sub-list just contains the head and tail nodes along with
   it's size, and there are basic private functions which belong to it as well. */


static void print_list(Linked_List_t *list, FILE *file, char *(*to_string)(void *item)){
	Node_t *node = NULL;
	int i = 0;
	char *all_items_in_list;
	asprintf(&all_items_in_list, "{ ");
	for(node = list->head; node ; node = node->next) {
		char *item_as_string = to_string(node->item);
		char *old_items_in_list = all_items_in_list;
		asprintf(&all_items_in_list, "%s  %s,", all_items_in_list, item_as_string);
		free(old_items_in_list);
		free(item_as_string);
	}
	char *old_items_in_list = all_items_in_list;
	asprintf(&all_items_in_list, "%s } Size: %d\n", all_items_in_list, list->size);
	fprintf(file, "%s\n", all_items_in_list);
	fflush(file);
	free(old_items_in_list);
	free(all_items_in_list);
}

static void swap_node_items(Node_t *node_one, Node_t *node_two){
	void *item = node_one->item;
	node_one->item = node_two->item;
	node_two->item = item;
}

static char *to_string(void *item){
	char *item_to_string;
	asprintf(&item_to_string, "%d", *(int *)item);
	return item_to_string;
}

static void insertion_sort_list(Linked_List_t *list, Linked_List_Compare compare){
	Node_t *node = NULL, *sub_node = NULL;
	for(node = list->head; node; node = node->next){
		void *item = node->item;
		sub_node = node->prev;
		while(sub_node && compare(sub_node->item, item) > 0){
			swap_node_items(sub_node->next, sub_node);
			sub_node = sub_node->prev;
		}
		if(sub_node) sub_node->next->item = item;
	}
}
/* Below are static private functions, or as I prefer to call them, helper functions, for the linked list. */


static int add_as_head(Linked_List_t *list, Node_t *node){
	node->next = list->head;
	list->head->prev = node;
	list->head = node;
	node->prev = NULL;
	return 1;
}

static int add_as_tail(Linked_List_t *list, Node_t *node){
	list->tail->next = node;
	node->prev = list->tail;
	list->tail = node;
	node->next = NULL;
	return 1;
}

static int add_between(Linked_List_t *list, Node_t *previous_node, Node_t *current_node){
	previous_node->prev->next = current_node;
	current_node->next = previous_node;
	current_node->prev = previous_node->prev;
	previous_node->prev = current_node;
	return 1;
}

static int add_after(Linked_List_t *list, Node_t *current_node, Node_t *new_node){
	current_node->next->prev = new_node;
	new_node->next = current_node->next;
	new_node->prev = current_node;
	current_node->next = new_node;
	return 1;
}

static int add_as_only(Linked_List_t *list, Node_t *node){
	list->head = list->tail = node;
	node->next = node->prev = NULL;
	list->size++;
	return 1;
}

static int add_before(Linked_List_t *list, Node_t *current_node, Node_t *new_node){
	current_node->prev->next = new_node;
	new_node->next = current_node;
	new_node->prev = current_node->prev;
	current_node->prev = new_node;
	return 1;
}

static int add_sorted(Linked_List_t *list, Node_t *node, Linked_List_Compare compare){
	if(!list->is_sorted) Linked_List_sort(list, compare);
	Node_t *current_node = NULL;
	if(list->size == 1) return compare(node->item, list->head->item) < 0 ? add_as_head(list, node) : add_as_tail(list, node);
	if(compare(node->item, list->head->item) <= 0) return add_as_head(list, node);
	if(compare(node->item, list->tail->item) >= 0) return add_as_tail(list, node);
	for(current_node = list->head; current_node; current_node = current_node->next){
		if(compare(node->item, current_node->item) <= 0) return add_before(list, current_node, node);
		else if(!current_node->next) return add_as_tail(list, node);
	}
	MU_LOG_ERROR(logger, "Was unable to add an item, sortedly, to the list!\n");
	return 0;
}

static int add_unsorted(Linked_List_t *list, Node_t *node){
	node->next = NULL;
	node->prev = list->tail;
	list->tail->next = node;
	list->tail = node;
	list->is_sorted = 0;
	return 1;
}

/// Lock before calling this function!
static int node_exists(Linked_List_t *list, Node_t *node){
	Node_t *temp_node = NULL;
	for(temp_node = list->head; temp_node; temp_node = temp_node->next) if(temp_node == node) return 1;
	return 0;
}

/// Obtains lock inside of function!
static int node_to_index(Linked_List_t *list, Node_t *node){
	int index = 0;
	Node_t *temp_node = NULL;
	for(temp_node = list->head; temp_node; temp_node = temp_node->next){
		index++;
		if(node == temp_node) {
			return index;
		}
	}
	MU_LOG_WARNING(logger, "Node_To_Index failed as the node was not found!\n");
	return 0;
}

/// Obtains lock inside of function!
static Node_t *item_to_node(Linked_List_t *list, void *item){
	if(list->head && list->head->item == item) return list->head;
	if(list->tail && list->tail->item == item) return list->tail;
	Node_t *node = NULL;
	for(node = list->head; node ; node = node->next) { 
		if(item == node->item) {
			return node;
		}
	}
	MU_LOG_WARNING(logger, "Item_To_Node was unable to find the item in the list, returning NULL");
	return NULL;
}

/// Obtains lock inside of function!
static Node_t *index_to_node(Linked_List_t *list, unsigned int index){
	if(index >= list->size) {
		return NULL;
	}
	if(index == (list->size-1)) {
		return list->tail;
	}
	if(index == 0) {
		return list->head;
	}
	if(index > (list->size / 2)){
		int i = list->size-1;
		Node_t *node = list->tail;
		while((node = node->prev) && --i != index);
		MU_ASSERT_RETURN(i == index, logger, NULL);
		return node;
	}
	int i = 0;
	Node_t *node = list->head;
	while((node = node->next) && ++i != index);
	MU_ASSERT_RETURN(i == index, logger, NULL);
	return node;
}

/// Obtains lock inside of function!
static int for_each_item(Linked_List_t *list, void (*callback)(void *item)){
	Node_t *node = NULL;
	for(node = list->head; node; node = node->next) callback(node->item);
	return 1;
}

/* Removes as if node is only one in list. */
static int remove_only(Linked_List_t *list, Node_t *node, Linked_List_Delete delete_item){
	list->head = NULL;
	list->tail = NULL;
	list->current = NULL;
	if(delete_item) delete_item(node->item);
	free(node);
	list->size = 0;
	return 1;
}

/* Removes as if node is the head one in the list. */
static int remove_head(Linked_List_t *list, Node_t *node, Linked_List_Delete delete_item){
	list->head->next->prev = NULL;
	list->head = list->head->next;
	if(list->current == node) list->current = list->head;
	if(delete_item) delete_item(node->item);
	free(node);
	list->size--;
	return 1;
}

/* Removes as if node is the tail one in the list. */
static int remove_tail(Linked_List_t *list, Node_t *node, Linked_List_Delete delete_item){
	list->tail = list->tail->prev;
	list->tail->next = NULL;
	if(list->current == node) list->current = list->tail;
	if(delete_item) delete_item(node->item);
	free(node);
	list->size--;
	return 1;
}

/* Removes as if, as is with the average case, this node is between two other nodes. */
static int remove_normal(Linked_List_t *list, Node_t *node, Linked_List_Delete delete_item){
	node->next->prev = node->prev;
	node->prev->next = node->next;
	if(delete_item) delete_item(node->item);
	if(list->current == node) list->current = node->next;
	free(node);
	list->size--;
	return 1;
}

static int remove_node(Linked_List_t *list, Node_t *node, Linked_List_Delete delete_item){
	int result = 0;
	if(list->size == 1) result = remove_only(list, node, delete_item);
	else if(list->tail == node) result = remove_tail(list, node, delete_item);
	else if(list->head == node) result = remove_head(list, node, delete_item);
	else result = remove_normal(list, node, delete_item);
	return result;
}

/// Obtains lock inside of function!
static int delete_all_nodes(Linked_List_t *list, Linked_List_Delete delete_item){
	Node_t *node = NULL;
	void *result = NULL;
	while(list->head) remove_node(list, list->head, delete_item);
	return 1;
}

/* End of private functions. */


Linked_List_t *Linked_List_create(void){
	if(!logger) logger = calloc(1, sizeof(MU_Logger_t));
	if(!logger) {
		MU_DEBUG("Unable to allocate memory for logger, Out of Memory!\n");
		return NULL;
	}
	MU_Logger_Init(logger, "Linked_List_Log.txt", "w", MU_ALL);
	Linked_List_t *list = malloc(sizeof(Linked_List_t));
	if(!list) {
		MU_DEBUG("See Log!!!\n");
		MU_LOG_ERROR(logger, "Was unable to allocate list, Out of Memory\n");
		MU_Logger_Deref(logger, 1);
		return NULL;
	}
	list->head = list->tail = list->current = NULL;
	list->size = 0;
	list->is_sorted = 1;
	list->manipulating_list = malloc(sizeof(pthread_rwlock_t));
	if(!list->manipulating_list){
		MU_DEBUG("See Log!!!\n");
		free(list);
		MU_LOG_ERROR(logger, "Unable to allocate manipulating_list, Out of Memory!\n");
		MU_Logger_Deref(logger, 1);
		return NULL;
	}
	list->manipulating_iterator = malloc(sizeof(pthread_rwlock_t));
	if(!list->manipulating_iterator){
		MU_DEBUG("See Log!!!\n");
		free(list->manipulating_list);
		free(list);
		MU_LOG_ERROR(logger, "Unable to allocate manipulating_iterator rwlock, Out of Memory!\n");
		MU_Logger_Deref(logger, 1);
		return NULL;
	}
	pthread_rwlock_init(list->manipulating_list, NULL);
	pthread_rwlock_init(list->manipulating_iterator, NULL);
	return list;
}

Linked_List_t *Linked_List_create_from(void **array, size_t size, Linked_List_Compare compare){
	Linked_List_t *list = Linked_List_create();
	int i = 0;
	for(;i<size;i++) Linked_List_add(list, array[i], compare);
	return list;
}

int Linked_List_add(Linked_List_t *list, void *item, Linked_List_Compare compare){
	if(!list) return 0;
	MU_ASSERT_RETURN(item, logger, 0);
	Node_t *node; 
	MU_ASSERT_RETURN(node = malloc(sizeof(Node_t)), logger, 0);
	node->item = item;
	pthread_rwlock_wrlock(list->manipulating_list);
	if(!list->size){
		add_as_only(list, node);
		pthread_rwlock_unlock(list->manipulating_list);
		return 1;
	}
	int result = 0;
	if(compare) result = add_sorted(list, node, compare);
	else result = add_unsorted(list, node);
	if(result) list->size++;
	pthread_rwlock_unlock(list->manipulating_list);
	return result;
}

void *Linked_List_get_at(Linked_List_t *list, unsigned int index){
	if(!list) return NULL;
	pthread_rwlock_rdlock(list->manipulating_list);
	Node_t *node = index_to_node(list, index);
	void *item = NULL;
	if(node) item = node->item;
	pthread_rwlock_unlock(list->manipulating_list);
	return item;
}

int Linked_List_for_each(Linked_List_t *list, void (*callback)(void *item)){
	if(!list || !callback) return 0;
	pthread_rwlock_rdlock(list->manipulating_list);
	int result = for_each_item(list, callback);
	pthread_rwlock_unlock(list->manipulating_list);
	return result;
}

int Linked_List_sort(Linked_List_t *list, Linked_List_Compare compare){
	if(!list || !list->size || !compare) return 0;
	pthread_rwlock_wrlock(list->manipulating_list);
	insertion_sort_list(list, compare);
	pthread_rwlock_unlock(list->manipulating_list);
	return 1;
}

int Linked_List_remove_item(Linked_List_t *list, void *item, Linked_List_Delete delete_item){
	if(!list) return 0;
	pthread_rwlock_wrlock(list->manipulating_list);
	Node_t *node = item_to_node(list, item);
	int result = 0;
	if(node) result = remove_node(list, node, delete_item);
	pthread_rwlock_unlock(list->manipulating_list);
	return result; 
}

void *Linked_List_remove_at(Linked_List_t *list, unsigned int index, Linked_List_Delete delete_item){
	if(!list) return 0;
	pthread_rwlock_wrlock(list->manipulating_list);
	Node_t *temp_node = index_to_node(list, index);
	void *item = NULL;
	if(temp_node){
		item = temp_node->item;
		remove_node(list, temp_node, delete_item);
	} else MU_LOG_WARNING(logger, "The node returned from Index_To_Node was NULL!\n");
	pthread_rwlock_unlock(list->manipulating_list);
	return item;
}

void *Linked_List_remove_current(Linked_List_t *list, Linked_List_Delete delete_item){
	if(!list || !list->current) return 0;
	pthread_rwlock_wrlock(list->manipulating_list);
	pthread_rwlock_wrlock(list->manipulating_iterator);
	void *item = list->current->item;
	remove_node(list, list->current, delete_item);
	pthread_rwlock_unlock(list->manipulating_iterator);
	pthread_rwlock_unlock(list->manipulating_list);
	return item;
}

void *Linked_List_next(Linked_List_t *list){
	if(!list) return NULL;
	pthread_rwlock_wrlock(list->manipulating_iterator);
	pthread_rwlock_rdlock(list->manipulating_list);
	if(!list->current || !list->current->next) {
		pthread_rwlock_unlock(list->manipulating_list);
		pthread_rwlock_unlock(list->manipulating_iterator);
		return NULL;
	}
	void *item = (list->current = list->current->next)->item;
	pthread_rwlock_unlock(list->manipulating_list);
	pthread_rwlock_unlock(list->manipulating_iterator);
	return item;
}

void * Linked_List_previous(Linked_List_t *list){
	if(!list) return NULL;
	pthread_rwlock_wrlock(list->manipulating_iterator);
	pthread_rwlock_rdlock(list->manipulating_list);
	if(!list->current || !list->current->prev) {
		pthread_rwlock_unlock(list->manipulating_list);
		pthread_rwlock_unlock(list->manipulating_iterator);
		return NULL;
	}
	void *item = (list->current = list->current->prev)->item;
	pthread_rwlock_unlock(list->manipulating_list);
	pthread_rwlock_unlock(list->manipulating_iterator);
	return item;
}

void * Linked_List_tail(Linked_List_t *list){
	if(!list) return NULL;
	pthread_rwlock_wrlock(list->manipulating_iterator);
	pthread_rwlock_rdlock(list->manipulating_list);
	if(!list->tail) {
		pthread_rwlock_unlock(list->manipulating_list);
		pthread_rwlock_unlock(list->manipulating_iterator);
		return NULL;
	}
	void *item = (list->current = list->tail)->item;
	pthread_rwlock_unlock(list->manipulating_list);
	pthread_rwlock_unlock(list->manipulating_iterator);
	return item;
}

void * Linked_List_head(Linked_List_t *list){
	if(!list) return NULL;
	pthread_rwlock_wrlock(list->manipulating_iterator);
	pthread_rwlock_rdlock(list->manipulating_list);
	if(!list->head) {
		pthread_rwlock_unlock(list->manipulating_list);
		pthread_rwlock_unlock(list->manipulating_iterator);
		return NULL;
	}
	void *item = (list->current = list->head)->item;
	pthread_rwlock_unlock(list->manipulating_list);
	pthread_rwlock_unlock(list->manipulating_iterator);
	return item;
}

void **Linked_List_to_array(Linked_List_t *list, size_t *size){
	if(!list) return NULL;
	pthread_rwlock_rdlock(list->manipulating_list);
	void **array_of_items = malloc(sizeof(void *) * list->size);
	MU_ASSERT_RETURN(array_of_items, logger, NULL);
	Node_t *node = NULL;
	int index = 0;
	for(node = list->head; node; node = node->next){
		array_of_items[index++] = node->item;
	}
	*size = index;
	pthread_rwlock_unlock(list->manipulating_list);
	return array_of_items;
}

int Linked_List_contains(Linked_List_t *list, void *item){
	if(!list) return 0;
	pthread_rwlock_rdlock(list->manipulating_list);
	Node_t *node = item_to_node(list, item);
	pthread_rwlock_unlock(list->manipulating_list);
	return node != NULL;
}

int Linked_List_add_after(Linked_List_t *list, void *item){
	if(!list) return 0;
	pthread_rwlock_wrlock(list->manipulating_list);
	pthread_rwlock_rdlock(list->manipulating_iterator);
	Node_t *node = malloc(sizeof(Node_t));
	node->item = item;
	if(!list->size){
		add_as_only(list, node);
		pthread_rwlock_unlock(list->manipulating_iterator);
		pthread_rwlock_unlock(list->manipulating_list);
		return 1;
	}
	if(list->current == list->tail) add_as_tail(list, node);
	else add_after(list, list->current, node);
	list->is_sorted = 0;
	pthread_rwlock_unlock(list->manipulating_iterator);
	pthread_rwlock_unlock(list->manipulating_list);
	return 1;
}

int Linked_List_add_before(Linked_List_t *list, void *item){
	if(!list) return 0;
	pthread_rwlock_wrlock(list->manipulating_list);
	pthread_rwlock_rdlock(list->manipulating_iterator);
	Node_t *node = malloc(sizeof(Node_t));
	node->item = item;
	if(!list->size){
		add_as_only(list, node);
		pthread_rwlock_unlock(list->manipulating_iterator);
		pthread_rwlock_unlock(list->manipulating_list);
		return 1;
	}
	if(list->current == list->head) add_as_head(list, node);
	else add_before(list, list->current, node);
	list->is_sorted = 0;
	pthread_rwlock_unlock(list->manipulating_iterator);
	pthread_rwlock_unlock(list->manipulating_list);
	return 1;
}

void Linked_List_print_all(Linked_List_t *list, FILE *file, char *(*to_string)(void *item)){
	if(!list || !file || !to_string) return;
	pthread_rwlock_rdlock(list->manipulating_list);
	print_list(list, file, to_string);
	pthread_rwlock_unlock(list->manipulating_list);
}

void *Linked_List_get_current(Linked_List_t *list){
	if(!list) return NULL;
	pthread_rwlock_rdlock(list->manipulating_list);
	pthread_rwlock_rdlock(list->manipulating_iterator);
	if(!list->current) {
		pthread_rwlock_unlock(list->manipulating_iterator);
		pthread_rwlock_unlock(list->manipulating_list);
		return NULL;
	}
	void *item = list->current->item;
	pthread_rwlock_unlock(list->manipulating_iterator);
	pthread_rwlock_unlock(list->manipulating_list);
	return item;

}

void Linked_List_destroy(Linked_List_t *list, Linked_List_Delete delete_item){
	if(!list) return;
	pthread_rwlock_wrlock(list->manipulating_list);
	pthread_rwlock_wrlock(list->manipulating_iterator);
	delete_all_nodes(list, delete_item);
	pthread_rwlock_unlock(list->manipulating_iterator);
	pthread_rwlock_unlock(list->manipulating_list);
	pthread_rwlock_destroy(list->manipulating_iterator);
	pthread_rwlock_destroy(list->manipulating_list);
	free(list->manipulating_list);
	free(list->manipulating_iterator);
	MU_Logger_Deref(logger,1);
	free(list);
}