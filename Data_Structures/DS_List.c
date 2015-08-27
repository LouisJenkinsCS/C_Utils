#include <DS_List.h>
#include <MU_Logger.h>
#include <MU_Arg_Check.h>
#include <MU_Cond_Locks.h>

/// Static logger for all linked lists do use.
static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("./Data_Structures/Logs/DS_List.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
}
/* Begin implementations of helper functions. */

/* Helper functions used for adding nodes to the list. */

static int add_as_head(DS_List_t *list, DS_Node_t *node){
	node->_double.next = list->head;
	list->head->_double.prev = node;
	list->head = node;
	node->_double.prev = NULL;
	list->size++;
	return 1;
}

static int add_as_tail(DS_List_t *list, DS_Node_t *node){
	list->tail->_double.next = node;
	node->_double.prev = list->tail;
	list->tail = node;
	node->_double.next = NULL;
	list->size++;
	return 1;
}

#if 0
static int add_between(DS_List_t *list, DS_Node_t *previous_node, DS_Node_t *current_node){
	previous_node->_double.prev->_double.next = current_node;
	current_node->_double.next = previous_node;
	current_node->_double.prev = previous_node->_double.prev;
	previous_node->_double.prev = current_node;
	list->size++;
	return 1;
}
#endif

static int add_after(DS_List_t *list, DS_Node_t *current_node, DS_Node_t *new_node){
	current_node->_double.next->_double.prev = new_node;
	new_node->_double.next = current_node->_double.next;
	new_node->_double.prev = current_node;
	current_node->_double.next = new_node;
	list->size++;
	return 1;
}

static int add_as_only(DS_List_t *list, DS_Node_t *node){
	list->head = list->tail = node;
	node->_double.next = node->_double.prev = NULL;
	list->size++;
	return 1;
}

static int add_before(DS_List_t *list, DS_Node_t *current_node, DS_Node_t *new_node){
	current_node->_double.prev->_double.next = new_node;
	new_node->_double.next = current_node;
	new_node->_double.prev = current_node->_double.prev;
	current_node->_double.prev = new_node;
	list->size++;
	return 1;
}

static int add_sorted(DS_List_t *list, DS_Node_t *node, DS_comparator_cb compare){
	if(!list->is_sorted) DS_List_sort(list, compare);
	DS_Node_t *current_node = NULL;
	if(list->size == 1) return compare(node->item, list->head->item) < 0 ? add_as_head(list, node) : add_as_tail(list, node);
	if(compare(node->item, list->head->item) <= 0) return add_as_head(list, node);
	if(compare(node->item, list->tail->item) >= 0) return add_as_tail(list, node);
	for(current_node = list->head; current_node; current_node = current_node->_double.next){
		if(compare(node->item, current_node->item) <= 0) return add_before(list, current_node, node);
		else if(!current_node->_double.next) return add_as_tail(list, node);
	}
	MU_LOG_ERROR(logger, "Was unable to add an item, sortedly, to the list!\n");
	return 0;
}

static int add_unsorted(DS_List_t *list, DS_Node_t *node){
	node->_double.next = NULL;
	node->_double.prev = list->tail;
	list->tail->_double.next = node;
	list->tail = node;
	list->is_sorted = 0;
	list->size++;
	return 1;
}

/* Helper Functions for removing items and nodes from the list. */

static int remove_only(DS_List_t *list, DS_Node_t *node, DS_delete_cb del){
	list->head = NULL;
	list->tail = NULL;
	list->current = NULL;
	if(del) del(node->item);
	free(node);
	list->size--;
	return 1;
}

static int remove_head(DS_List_t *list, DS_Node_t *node, DS_delete_cb del){
	list->head->_double.next->_double.prev = NULL;
	list->head = list->head->_double.next;
	if(list->current == node) list->current = list->head;
	if(del) del(node->item);
	free(node);
	list->size--;
	return 1;
}

static int remove_tail(DS_List_t *list, DS_Node_t *node, DS_delete_cb del){
	list->tail = list->tail->_double.prev;
	list->tail->_double.next = NULL;
	if(list->current == node) list->current = list->tail;
	if(del) del(node->item);
	free(node);
	list->size--;
	return 1;
}

static int remove_normal(DS_List_t *list, DS_Node_t *node, DS_delete_cb del){
	node->_double.next->_double.prev = node->_double.prev;
	node->_double.prev->_double.next = node->_double.next;
	if(del) del(node->item);
	if(list->current == node) list->current = node->_double.next;
	free(node);
	list->size--;
	return 1;
}

static int remove_node(DS_List_t *list, DS_Node_t *node, DS_delete_cb del){
	int result = 0;
	if(list->size == 1) result = remove_only(list, node, del);
	else if(list->tail == node) result = remove_tail(list, node, del);
	else if(list->head == node) result = remove_head(list, node, del);
	else result = remove_normal(list, node, del);
	return result;
}

/* Helper functions for sorting the linked list */

static void swap_node_items(DS_Node_t *node_one, DS_Node_t *node_two){
	void *item = node_one->item;
	node_one->item = node_two->item;
	node_two->item = item;
}

static void insertion_sort_list(DS_List_t *list, DS_comparator_cb compare){
	DS_Node_t *node = NULL, *sub_node = NULL;
	for(node = list->head; node; node = node->_double.next){
		void *item = node->item;
		sub_node = node->_double.prev;
		while(sub_node && compare(sub_node->item, item) > 0){
			swap_node_items(sub_node->_double.next, sub_node);
			sub_node = sub_node->_double.prev;
		}
		if(sub_node) sub_node->_double.next->item = item;
	}
}

/* Helper functions for general linked list operations. */

static void print_list(DS_List_t *list, FILE *file, DS_to_string_cb to_string){
	DS_Node_t *node = NULL;
	char *all_items_in_list;
	asprintf(&all_items_in_list, "{ ");
	for(node = list->head; node ; node = node->_double.next) {
		char *item_as_string = to_string(node->item);
		char *old_items_in_list = all_items_in_list;
		asprintf(&all_items_in_list, "%s  %s,", all_items_in_list, item_as_string);
		free(old_items_in_list);
		free(item_as_string);
	}
	char *old_items_in_list = all_items_in_list;
	asprintf(&all_items_in_list, "%s } Size: %zu\n", all_items_in_list, list->size);
	fprintf(file, "%s\n", all_items_in_list);
	fflush(file);
	free(old_items_in_list);
	free(all_items_in_list);
}

static int delete_all_nodes(DS_List_t *list, DS_delete_cb del){
	while(list->head) remove_node(list, list->head, del);
	return 1;
}

#if 0
static int node_exists(DS_List_t *list, DS_Node_t *node){
	DS_Node_t *temp_node = NULL;
	for(temp_node = list->head; temp_node; temp_node = temp_node->_double.next) if(temp_node == node) return 1;
	return 0;
}

static int node_to_index(DS_List_t *list, DS_Node_t *node){
	int index = 0;
	DS_Node_t *temp_node = NULL;
	for(temp_node = list->head; temp_node; temp_node = temp_node->_double.next){
		index++;
		if(node == temp_node) {
			return index;
		}
	}
	MU_LOG_WARNING(logger, "Node_To_Index failed as the node was not found!\n");
	return 0;
}
#endif


static DS_Node_t *item_to_node(DS_List_t *list, void *item){
	if(list->head && list->head->item == item) return list->head;
	if(list->tail && list->tail->item == item) return list->tail;
	DS_Node_t *node = NULL;
	for(node = list->head; node ; node = node->_double.next) { 
		if(item == node->item) {
			return node;
		}
	}
	return NULL;
}

static DS_Node_t *index_to_node(DS_List_t *list, unsigned int index){
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
		DS_Node_t *node = list->tail;
		while((node = node->_double.prev) && --i != index);
		MU_ASSERT(i == index, logger, "Error in Node Traversal!Expected index %u, stopped at index %d!", index, i);
		return node;
	}
	int i = 0;
	DS_Node_t *node = list->head;
	while((node = node->_double.next) && ++i != index);
	MU_ASSERT(i == index, logger, "Error in Node Traversal!Expected index %u, stopped at index %d!", index, i);
	return node;
}

static void for_each_item(DS_List_t *list, void (*callback)(void *item)){
	DS_Node_t *node = NULL;
	for(node = list->head; node; node = node->_double.next) callback(node->item);
}

/* End implementations of helper functions. */

/* Linked List Creation and Deletion functions */

DS_List_t *DS_List_create(bool synchronized){
	DS_List_t *list = calloc(1, sizeof(DS_List_t));
	if(!list) {
		MU_DEBUG("See Log!!!\n");
		MU_LOG_ERROR(logger, "Was unable to allocate list, Out of Memory\n");
		return NULL;
	}
	list->head = list->tail = list->current = NULL;
	list->size = 0;
	list->is_sorted = 1;
	if(synchronized){
		list->manipulating_list = malloc(sizeof(pthread_rwlock_t));
		if(!list->manipulating_list){
			MU_DEBUG("See Log!!!\n");
			free(list);
			MU_LOG_ERROR(logger, "Unable to allocate manipulating_list, Out of Memory!\n");
			return NULL;
		}
		list->manipulating_iterator = malloc(sizeof(pthread_rwlock_t));
		if(!list->manipulating_iterator){
			MU_DEBUG("See Log!!!\n");
			free(list->manipulating_list);
			free(list);
			MU_LOG_ERROR(logger, "Unable to allocate manipulating_iterator rwlock, Out of Memory!\n");
			return NULL;
		}
		pthread_rwlock_init(list->manipulating_list, NULL);
		pthread_rwlock_init(list->manipulating_iterator, NULL);
	}
	return list;
}

DS_List_t *DS_List_create_from(void **array, size_t size, DS_comparator_cb compare, bool synchronized){
	MU_ARG_CHECK(logger, NULL, array);
	DS_List_t *list = DS_List_create(synchronized);
	int i = 0;
	for(;i<size;i++) DS_List_add(list, array[i], compare);
	return list;
}

bool DS_List_clear(DS_List_t *list, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, list);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_iterator, logger);
	delete_all_nodes(list, del);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return true;
}

bool DS_List_destroy(DS_List_t *list, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, list);
	DS_List_clear(list, del);
	MU_COND_RWLOCK_DESTROY(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_DESTROY(list->manipulating_list, logger);
	free(list);
	return true;
}

/* Linked List adding functions */

bool DS_List_add(DS_List_t *list, void *item, DS_comparator_cb compare){
	MU_ARG_CHECK(logger, false, list, item);
	DS_Node_t *node = malloc(sizeof(DS_Node_t)); 
	if(!node){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		return false;
	}
	node->item = item;
	MU_COND_RWLOCK_WRLOCK(list->manipulating_list, logger);
	if(!list->size){
		add_as_only(list, node);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
		return true;
	}
	bool result = false;
	if(compare) result = add_sorted(list, node, compare);
	else result = add_unsorted(list, node);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return result;
}

bool DS_List_add_after(DS_List_t *list, void *item){
	MU_ARG_CHECK(logger, false, list, item);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_iterator, logger);
	DS_Node_t *node = malloc(sizeof(DS_Node_t));
	node->item = item;
	if(!list->size){
		add_as_only(list, node);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
		return 1;
	}
	if(list->current == list->tail) add_as_tail(list, node);
	else add_after(list, list->current, node);
	list->is_sorted = 0;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return 1;
}

bool DS_List_add_before(DS_List_t *list, void *item){
	MU_ARG_CHECK(logger, false, list, item);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_iterator, logger);
	DS_Node_t *node = malloc(sizeof(DS_Node_t));
	node->item = item;
	if(!list->size){
		add_as_only(list, node);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
		return 1;
	}
	if(list->current == list->head) add_as_head(list, node);
	else add_before(list, list->current, node);
	list->is_sorted = 0;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return 1;
}

/* Linked List removal functions */

bool DS_List_remove_item(DS_List_t *list, void *item, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, list, item);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_list, logger);
	DS_Node_t *node = item_to_node(list, item);
	int result = 0;
	if(node) result = remove_node(list, node, del);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return result; 
}

void *DS_List_remove_at(DS_List_t *list, unsigned int index, DS_delete_cb del){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_list, logger);
	DS_Node_t *temp_node = index_to_node(list, index);
	void *item = NULL;
	if(temp_node){
		item = temp_node->item;
		remove_node(list, temp_node, del);
	} else MU_LOG_WARNING(logger, "The node returned from Index_To_Node was NULL!\n");
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return item;
}

/* Linked List iterator functions */

void *DS_List_remove_current(DS_List_t *list, DS_delete_cb del){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_iterator, logger);
	void *item = list->current->item;
	remove_node(list, list->current, del);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return item;
}

void *DS_List_next(DS_List_t *list){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	if(!list->current || !list->current->_double.next) {
		MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
		return NULL;
	}
	void *item = (list->current = list->current->_double.next)->item;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	return item;
}

void * DS_List_previous(DS_List_t *list){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	if(!list->current || !list->current->_double.prev) {
		MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
		return NULL;
	}
	void *item = (list->current = list->current->_double.prev)->item;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	return item;
}

void * DS_List_tail(DS_List_t *list){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	if(!list->tail) {
		MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
		return NULL;
	}
	void *item = (list->current = list->tail)->item;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	return item;
}

void * DS_List_head(DS_List_t *list){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	if(!list->head) {
		MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
		return NULL;
	}
	void *item = (list->current = list->head)->item;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	return item;
}

void *DS_List_get_current(DS_List_t *list){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_iterator, logger);
	if(!list->current) {
		MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
		MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
		return NULL;
	}
	void *item = list->current->item;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_iterator, logger);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return item;
}

/* Linked List miscallaneous functions */

bool DS_List_for_each(DS_List_t *list, void (*callback)(void *item)){
	MU_ARG_CHECK(logger, false, list, callback);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	for_each_item(list, callback);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return true;
}

bool DS_List_sort(DS_List_t *list, DS_comparator_cb compare){
	MU_ARG_CHECK(logger, false, list, compare);
	MU_COND_RWLOCK_WRLOCK(list->manipulating_list, logger);
	insertion_sort_list(list, compare);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return true;
}

bool DS_List_contains(DS_List_t *list, void *item){
	MU_ARG_CHECK(logger, false, list, item);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	DS_Node_t *node = item_to_node(list, item);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return node != NULL;
}

bool DS_List_print_all(DS_List_t *list, FILE *file, DS_to_string_cb to_string){
	MU_ARG_CHECK(logger, false, list, file, to_string);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	print_list(list, file, to_string);
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return true;
}

void *DS_List_get_at(DS_List_t *list, unsigned int index){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	DS_Node_t *node = index_to_node(list, index);
	void *item = NULL;
	if(node) item = node->item;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return item;
}

void **DS_List_to_array(DS_List_t *list, size_t *size){
	MU_ARG_CHECK(logger, NULL, list);
	MU_COND_RWLOCK_RDLOCK(list->manipulating_list, logger);
	void **array_of_items = malloc(sizeof(void *) * list->size);
	if(!array_of_items){
		MU_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		return NULL;
	}
	DS_Node_t *node = NULL;
	int index = 0;
	for(node = list->head; node; node = node->_double.next){
		array_of_items[index++] = node->item;
	}
	*size = index;
	MU_COND_RWLOCK_UNLOCK(list->manipulating_list, logger);
	return array_of_items;
}
