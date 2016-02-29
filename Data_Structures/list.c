#include "../data_structures/list.h"
#include "../io/logger.h"
#include "../misc/argument_check.h"
#include "../misc/flags.h"
#include "../threading/scoped_lock.h"

struct c_utils_list {
	/// The head node of the list.
	struct c_utils_node *head;
	/// The tail node of the list.
	struct c_utils_node *tail;
	/// The current size of the linked list.
	volatile size_t size;
	/// Determines whether the list is sorted.
	volatile unsigned char is_sorted;
	/// Ensures only one thread manipulates the items in the list, but multiple threads can read.
	struct c_utils_scoped_lock *lock;
};

/// Static logger for all linked lists do use.
static struct c_utils_logger *logger = NULL;

C_UTILS_LOGGER_AUTO_CREATE(logger, "./data_structures/logs/list.log", "w", C_UTILS_LOG_LEVEL_ALL);

/* Begin implementations of helper functions. */

/* Helper functions used for adding nodes to the list. */

static inline int add_as_head(struct c_utils_list *list, struct c_utils_node *node) {
	node->_double.next = list->head;
	list->head->_double.prev = node;
	list->head = node;
	node->_double.prev = NULL;
	list->size++;
	
	return 1;
}

static inline int add_as_tail(struct c_utils_list *list, struct c_utils_node *node) {
	list->tail->_double.next = node;
	node->_double.prev = list->tail;
	list->tail = node;
	node->_double.next = NULL;
	list->size++;
	
	return 1;
}

static inline int add_after(struct c_utils_list *list, struct c_utils_node *current_node, struct c_utils_node *new_node) {
	current_node->_double.next->_double.prev = new_node;
	new_node->_double.next = current_node->_double.next;
	new_node->_double.prev = current_node;
	current_node->_double.next = new_node;
	list->size++;
	
	return 1;
}


static inline int add_as_only(struct c_utils_list *list, struct c_utils_node *node) {
	list->head = list->tail = node;
	node->_double.next = node->_double.prev = NULL;
	list->size++;
	
	return 1;
}

static inline int add_before(struct c_utils_list *list, struct c_utils_node *current_node, struct c_utils_node *new_node) {
	current_node->_double.prev->_double.next = new_node;
	new_node->_double.next = current_node;
	new_node->_double.prev = current_node->_double.prev;
	current_node->_double.prev = new_node;
	list->size++;
	
	return 1;
}

static inline int add_sorted(struct c_utils_list *list, struct c_utils_node *node, c_utils_comparator_cb compare) {
	if (!list->is_sorted) 
		c_utils_list_sort(list, compare);

	struct c_utils_node *current_node = NULL;
	if (list->size == 1)
		return compare(node->item, list->head->item) < 0 ? add_as_head(list, node) : add_as_tail(list, node);

	if (compare(node->item, list->head->item) <= 0)
		return add_as_head(list, node);

	if (compare(node->item, list->tail->item) >= 0)
		return add_as_tail(list, node);

	for (current_node = list->head; current_node; current_node = current_node->_double.next) {
		if (compare(node->item, current_node->item) <= 0)
			return add_before(list, current_node, node);
		else if (!current_node->_double.next)
			return add_as_tail(list, node);
	}
	C_UTILS_LOG_ERROR(logger, "Was unable to add an item, sortedly, to the list!\n");
	
	return 0;
}

static inline int add_unsorted(struct c_utils_list *list, struct c_utils_node *node) {
	node->_double.next = NULL;
	node->_double.prev = list->tail;
	list->tail->_double.next = node;
	list->tail = node;
	list->is_sorted = 0;
	list->size++;

	return 1;
}

/* Helper Functions for removing items and nodes from the list. */

static inline int remove_only(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del) {
	list->head = NULL;
	list->tail = NULL;
	
	if (del)
		del(node->item);
	free(node);
	list->size--;
	
	return 1;
}

static inline int remove_head(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del) {
	list->head->_double.next->_double.prev = NULL;
	list->head = list->head->_double.next;

	if (del)
		del(node->item);
	free(node);
	list->size--;
	
	return 1;
}

static inline int remove_tail(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del) {
	list->tail = list->tail->_double.prev;
	list->tail->_double.next = NULL;
	
	if (del)
		del(node->item);
	free(node);
	list->size--;
	
	return 1;
}

static inline int remove_normal(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del) {
	node->_double.next->_double.prev = node->_double.prev;
	node->_double.prev->_double.next = node->_double.next;
	
	if (del)
		del(node->item);
	free(node);
	list->size--;
	
	return 1;
}

static inline int remove_node(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del) {
	if (list->size == 1)
		return remove_only(list, node, del);
	else if (list->tail == node)
		return remove_tail(list, node, del);
	else if (list->head == node)
		return remove_head(list, node, del);
	else
		return remove_normal(list, node, del);
}

/* Helper functions for sorting the linked list */

static inline void swap_node_items(struct c_utils_node *node_one, struct c_utils_node *node_two) {
	void *item = node_one->item;
	node_one->item = node_two->item;
	node_two->item = item;
}

static inline void insertion_sort_list(struct c_utils_list *list, c_utils_comparator_cb compare) {
	struct c_utils_node *node = NULL, *sub_node = NULL;
	for (node = list->head; node; node = node->_double.next) {
		void *item = node->item;
		
		sub_node = node->_double.prev;
		while (sub_node && compare(sub_node->item, item) > 0) {
			swap_node_items(sub_node->_double.next, sub_node);
			sub_node = sub_node->_double.prev;
		}
		
		if (sub_node)
			sub_node->_double.next->item = item;
	}
}

/* Helper functions for general linked list operations. */

static void print_list(struct c_utils_list *list, FILE *file, c_utils_to_string_cb to_string) {
	struct c_utils_node *node = NULL;
	char *all_items_in_list;
	
	asprintf(&all_items_in_list, "{ ");
	for (node = list->head; node ; node = node->_double.next) {
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

static int delete_all_nodes(struct c_utils_list *list, c_utils_delete_cb del) {
	while (list->head)
		remove_node(list, list->head, del);
	
	return 1;
}

#if 0
static int node_exists(struct c_utils_list *list, struct c_utils_node *node) {
	struct c_utils_node *temp_node = NULL;
	for (temp_node = list->head; temp_node; temp_node = temp_node->_double.next) 
		if (temp_node == node) 
			return 1;
	return 0;
}

static int node_to_index(struct c_utils_list *list, struct c_utils_node *node) {
	int index = 0;
	struct c_utils_node *temp_node = NULL;
	for (temp_node = list->head; temp_node; temp_node = temp_node->_double.next) {
		index++;
		if (node == temp_node)
			return index;
	}
	C_UTILS_LOG_WARNING(logger, "Node_To_Index failed as the node was not found!\n");
	return 0;
}
#endif


static struct c_utils_node *item_to_node(struct c_utils_list *list, void *item) {
	if (list->head && list->head->item == item)
		return list->head;
	
	if (list->tail && list->tail->item == item)
		return list->tail;
	
	struct c_utils_node *node = NULL;
	for (node = list->head; node ; node = node->_double.next)
		if (item == node->item)
			return node;
	
	return NULL;
}

static struct c_utils_node *index_to_node(struct c_utils_list *list, unsigned int index) {
	if (index >= list->size)
		return NULL;
	
	if (index == (list->size - 1))
		return list->tail;
	
	if (index == 0)
		return list->head;
	
	struct c_utils_node *node;
	if (index > (list->size / 2)) {
		int i = list->size-1;
		node = list->tail;
		while ((node = node->_double.prev) && --i != index) ;
		C_UTILS_ASSERT(i == index, logger, "Error in Node Traversal!Expected index %u, stopped at index %d!", index, i);
		return node;
	} else {
		int i = 0;
		node = list->head;
		while ((node = node->_double.next) && ++i != index);
		C_UTILS_ASSERT(i == index, logger, "Error in Node Traversal!Expected index %u, stopped at index %d!", index, i);
	}
	
	return node;
}

static void for_each_item(struct c_utils_list *list, void (*callback)(void *item)) {
	struct c_utils_node *node = NULL;
	for (node = list->head; node; node = node->_double.next)
		callback(node->item);
}

/* End implementations of helper functions. */

/* Implementation of c_utils_iterator callbacks */

static const int curr_valid = 1 << 0;
static const int prev_valid = 1 << 1;
static const int next_valid = 1 << 2;

static unsigned int is_valid(struct c_utils_list *list, struct c_utils_position *pos) {
	unsigned int retval = 0;
	struct c_utils_node *tmp;
	for (tmp = list->head; tmp; tmp = tmp->_double.next) {
		if (tmp == pos->curr) {
			C_UTILS_FLAG_SET(retval, curr_valid);
		} else if (tmp == pos->prev) {
			C_UTILS_FLAG_SET(retval, prev_valid);
		} else if (tmp == pos->next) {
			C_UTILS_FLAG_SET(retval, next_valid);
			break;
		}
	}

	return retval;
}

static inline void *get_item(struct c_utils_node *node) {
	return node ? node->item : NULL;
}

static void update_pos(struct c_utils_position *pos, struct c_utils_node *node) {
	pos->curr = node;
	pos->next = node ? node->_double.next : NULL;
	pos->prev = node ? node->_double.prev : NULL;
}

static void *head(void *instance, struct c_utils_position *pos) {
	struct c_utils_list *list = instance;
	struct c_utils_node *head;

	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock) {
		head = list->head;
		update_pos(pos, head);
		return get_item(head);
	} // Release Reader Lock

	__builtin_unreachable();
}

static void *tail(void *instance, struct c_utils_position *pos) {
	struct c_utils_list *list = instance;
	struct c_utils_node *tail;

	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock) {
		tail = list->tail;
		update_pos(pos, tail);
		return get_item(tail);
	} // Release Reader Lock

	__builtin_unreachable();
}

static void *next(void *instance, struct c_utils_position *pos) {
	struct c_utils_list *list = instance;
	struct c_utils_node *next = NULL;

	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock) {
		if (list->size == 0) {
			update_pos(pos, NULL);
			return NULL;
		}

		if (!pos->curr) {
			next = list->head;
			update_pos(pos, next);
			return get_item(next);
		}

		unsigned int are_valid = is_valid(list, pos);
		if (C_UTILS_FLAG_GET(are_valid, curr_valid)) {
			next = pos->curr->_double.next;
		} else if (C_UTILS_FLAG_GET(are_valid, next_valid)) {
			next = pos->next;
		} else if (C_UTILS_FLAG_GET(are_valid, prev_valid)) {
			next = pos->prev;
			if (next)
				next = next->_double.next;
			else
				next = list->head;
		} else {
			next = list->head;
		}
		update_pos(pos, next);
		
		return get_item(next);
	} // Release Reader Lock

	__builtin_unreachable();
}

static void *prev(void *instance, struct c_utils_position *pos) {
	struct c_utils_list *list = instance;
	struct c_utils_node *prev;
	
	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock) {
		if (list->size == 0) {
			update_pos(pos, NULL);
			return NULL;
		}

		if (!pos->curr) {
			prev = list->tail;
			update_pos(pos, prev);
			return get_item(prev);;
		}

		unsigned int are_valid = is_valid(list, pos);
		if (C_UTILS_FLAG_GET(are_valid, curr_valid)) {
			prev = pos->curr->_double.prev;
		} else if (C_UTILS_FLAG_GET(are_valid, prev_valid)) {
			prev = pos->prev;
		} else if (C_UTILS_FLAG_GET(are_valid, next_valid)) {
			prev = pos->next;
			if (prev)
				prev = prev->_double.prev;
			else 
				prev = list->tail;
		} else {
			prev = list->tail;
		}
		update_pos(pos, prev);
		
		return get_item(prev);
	} // Release Reader Lock

	__builtin_unreachable();
}

static bool append(void *instance, struct c_utils_position *pos, void *item) {
	struct c_utils_list *list = instance;
	
	struct c_utils_node *node = malloc(sizeof(*node));
	if (!node) {
		C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		return false;
	}
	node->item = item;
	
	// Acquire Writer Lock
	SCOPED_LOCK0(list->lock) {
		if (list->size == 0) {
			add_as_only(list, node);
			update_pos(pos, node);
			return true;
		}

		// If current is NULL, we append it to the end.
		if (!pos->curr) {
			add_as_tail(list, node);
			update_pos(pos, node);
			return true;
		}
		
		unsigned int are_valid = is_valid(list, pos);
		if (C_UTILS_FLAG_GET(are_valid, curr_valid)) {
			if (pos->curr->_double.next)
				add_after(list, pos->curr, node);
			else
				add_as_tail(list, node);
		} else if (C_UTILS_FLAG_GET(are_valid, next_valid)) {
			if (pos->next->_double.next)
				add_after(list, pos->next, node);
			else
				add_as_tail(list, node);
		} else if (C_UTILS_FLAG_GET(are_valid, prev_valid)) {
			if (pos->prev->_double.next)
				add_after(list, pos->prev, node);
			else
				add_as_tail(list, node);
		} else {
			add_as_tail(list, node);
		}
		update_pos(pos, node);

		return true;
	} // Release Writer Lock

	__builtin_unreachable();
}

static bool prepend(void *instance, struct c_utils_position *pos, void *item) {
	struct c_utils_list *list = instance;
	
	struct c_utils_node *node = malloc(sizeof(*node));
	if (!node) {
		C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		return false;
	}
	node->item = item;
	
	// Acquire Writer Lock
	SCOPED_LOCK0(list->lock) {
		if (list->size == 0) {
			add_as_only(list, node);
			update_pos(pos, node);
			return true; 
		}

		// If current is NULL, we prepend to the beginning.
		if (!pos->curr) {
			add_as_tail(list, node);
			update_pos(pos, node);
			return true;
		}

		unsigned int are_valid = is_valid(list, pos);
		if (C_UTILS_FLAG_GET(are_valid, curr_valid)) {
			if (pos->curr->_double.prev)
				add_before(list, pos->curr, node);
			else 
				add_as_head(list, node);
		} else if (C_UTILS_FLAG_GET(are_valid, next_valid)) {
			if (pos->next->_double.prev)
				add_before(list, pos->next, node);
			else
				add_as_head(list, node);
		} else if (C_UTILS_FLAG_GET(are_valid, prev_valid)) {
			if (pos->prev->_double.prev)
				add_before(list, pos->prev, node);
			else
				add_as_head(list, node);
		} else {
			add_as_head(list, node);
		}
		update_pos(pos, node);

		return true;
	} // Release Writer Lock

	__builtin_unreachable();
}

/* Linked List Creation and Deletion functions */

struct c_utils_list *c_utils_list_create(bool synchronized) {
	struct c_utils_list *list = calloc(1, sizeof(struct c_utils_list));
	if (!list) {
		C_UTILS_DEBUG("See Log!!!\n");
		C_UTILS_LOG_ERROR(logger, "Was unable to allocate list, Out of Memory\n");
		return NULL;
	}

	list->head = list->tail = NULL;
	list->size = 0;
	list->is_sorted = 1;

	if (synchronized)
		list->lock = c_utils_scoped_lock_rwlock(NULL, logger);
	else
		list->lock = c_utils_scoped_lock_no_op();

	if (!list->lock) {
		C_UTILS_LOG_ERROR(logger, "Was unable to create scoped_lock for rwlock!");
		free(list);
		return NULL;
	}

	return list;
}

struct c_utils_list *c_utils_list_from(void **array, size_t size, c_utils_comparator_cb compare, bool synchronized) {
	C_UTILS_ARG_CHECK(logger, NULL, array);
	
	struct c_utils_list *list = c_utils_list_create(synchronized);
	
	int i = 0;
	for (;i<size;i++)
		c_utils_list_add(list, array[i], compare);

	return list;
}

void c_utils_list_clear(struct c_utils_list *list, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, , list);
	
	// Acquire Writer Lock
	SCOPED_LOCK0(list->lock) 
		delete_all_nodes(list, del);
}

void c_utils_list_destroy(struct c_utils_list *list, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, , list);

	c_utils_list_clear(list, del);

	c_utils_scoped_lock_destroy(list->lock);
	free(list);
}

/* Linked List adding functions */

bool c_utils_list_add(struct c_utils_list *list, void *item, c_utils_comparator_cb compare) {
	C_UTILS_ARG_CHECK(logger, false, list, item);
	
	struct c_utils_node *node = malloc(sizeof(struct c_utils_node));
	if (!node) {
		C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
		return false;
	}
	node->item = item;
	
	// Acquire Writer Lock
	SCOPED_LOCK0(list->lock) {
		if (!list->size)
			return add_as_only(list, node);

		if (compare)
			return add_sorted(list, node, compare);
		else
			return add_unsorted(list, node);
	} // Release Writer Lock

	__builtin_unreachable();
}

/* Linked List removal functions */

bool c_utils_list_remove(struct c_utils_list *list, void *item, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, false, list, item);

	// Acquire Writer Lock
	SCOPED_LOCK0(list->lock) {
		struct c_utils_node *node = item_to_node(list, item);

		return node ? remove_node(list, node, del) : false;
	} // Release Writer Lock

	__builtin_unreachable();
}

void *c_utils_list_remove_at(struct c_utils_list *list, unsigned int index, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, NULL, list);

	// Acquire Writer Lock
	SCOPED_LOCK0(list->lock) {
		struct c_utils_node *temp_node = index_to_node(list, index);
		
		if (temp_node) {
			void *item = temp_node->item;
			remove_node(list, temp_node, del);
			return item;
		} else {
			C_UTILS_LOG_WARNING(logger, "The node returned from Index_To_Node was NULL!\n");
			return NULL;
		}
	} // Release Writer Lock

	__builtin_unreachable();
}

/* Linked List miscallaneous functions */

size_t c_utils_list_size(struct c_utils_list *list) {
	return list->size;
}

bool c_utils_list_for_each(struct c_utils_list *list, void (*callback)(void *item)) {
	C_UTILS_ARG_CHECK(logger, false, list, callback);

	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock)
		for_each_item(list, callback);
		
	return true;
}

bool c_utils_list_sort(struct c_utils_list *list, c_utils_comparator_cb compare) {
	C_UTILS_ARG_CHECK(logger, false, list, compare);

	// Acquire Writer Lock
	SCOPED_LOCK0(list->lock)
		insertion_sort_list(list, compare);

	return true;
}

bool c_utils_list_contains(struct c_utils_list *list, void *item) {
	C_UTILS_ARG_CHECK(logger, false, list, item);

	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock)
		return !!item_to_node(list, item);

	__builtin_unreachable();
}

bool c_utils_list_print(struct c_utils_list *list, FILE *file, c_utils_to_string_cb to_string) {
	C_UTILS_ARG_CHECK(logger, false, list, file, to_string);

	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock)
		print_list(list, file, to_string);

	return true;
}

void *c_utils_list_get(struct c_utils_list *list, unsigned int index) {
	C_UTILS_ARG_CHECK(logger, NULL, list);

	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock) {
		struct c_utils_node *node = index_to_node(list, index);
		return node ? node->item : NULL;
	} // Release Reader Lock

	__builtin_unreachable();
}

void **c_utils_list_as_array(struct c_utils_list *list, size_t *size) {
	C_UTILS_ARG_CHECK(logger, NULL, list);

	// Acquire Reader Lock
	SCOPED_LOCK1(list->lock) {
		void **array_of_items = malloc(sizeof(void *) * list->size);
		if (!array_of_items) {
			C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			return NULL;
		}

		struct c_utils_node *node = NULL;
		int index = 0;
		for (node = list->head; node; node = node->_double.next)
			array_of_items[index++] = node->item;

		*size = index;
		return array_of_items;
	} // Release Reader Lock

	__builtin_unreachable();
}


struct c_utils_iterator *c_utils_list_iterator(struct c_utils_list *list) {
	struct c_utils_iterator *it = calloc(1, sizeof(*it));
	if (!it) {
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		goto error;
	}

	it->handle = list;
	it->head = head;
	it->tail = tail;
	it->next = next;
	it->prev = prev;
	it->append = append;
	it->prepend = prepend;
	
	return it;

	error:
		free(it);
		return NULL;
}