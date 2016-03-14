#include "../data_structures/list.h"
#include "../io/logger.h"
#include "../misc/flags.h"
#include "../misc/alloc_check.h"
#include "../threading/scoped_lock.h"
#include "../misc/argument_check.h"
#include "../memory/ref_count.h"

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
	/// The configuration object used to retrieve callbacks and flags.
	struct c_utils_list_conf conf;
};


//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						List Add Helper Functions                                   //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////


static inline int add_as_head(struct c_utils_list *list, struct c_utils_node *node);

static inline int add_as_tail(struct c_utils_list *list, struct c_utils_node *node);

static inline int add_after(struct c_utils_list *list, struct c_utils_node *current_node, struct c_utils_node *new_node);

static inline int add_as_only(struct c_utils_list *list, struct c_utils_node *node);

static inline int add_before(struct c_utils_list *list, struct c_utils_node *current_node, struct c_utils_node *new_node);

static inline int add_sorted(struct c_utils_list *list, struct c_utils_node *node, c_utils_comparator_cb compare);

static inline int add_unsorted(struct c_utils_list *list, struct c_utils_node *node);


//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						List Removal Helper Functions                               //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////


static inline int remove_only(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del);

static inline int remove_head(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del);

static inline int remove_tail(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del);

static inline int remove_normal(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del);

static inline int remove_node(struct c_utils_list *list, struct c_utils_node *node, c_utils_delete_cb del);

static void *remove_at(struct c_utils_list *list, unsigned int index, bool delete_item);

static void remove_item(struct c_utils_list *list, void *item, bool delete_item);


//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						List Misc Helper Functions                                  //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////


static inline void insertion_sort_list(struct c_utils_list *list, c_utils_comparator_cb compare);

static inline void swap_node_items(struct c_utils_node *node_one, struct c_utils_node *node_two);

static int delete_all_nodes(struct c_utils_list *list, c_utils_delete_cb del);

static struct c_utils_node *item_to_node(struct c_utils_list *list, void *item);

static struct c_utils_node *index_to_node(struct c_utils_list *list, unsigned int index);

static void for_each_item(struct c_utils_list *list, void (*callback)(void *item));

static void destroy_list(void *instance);


//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						Iterator Implementation functions                           //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////

static unsigned int is_valid(struct c_utils_list *list, struct c_utils_position *pos);

static inline void *get_item(struct c_utils_node *node);

static void update_pos(struct c_utils_position *pos, struct c_utils_node *node);

static void *head(void *instance, struct c_utils_position *pos);

static void *tail(void *instance, struct c_utils_position *pos);

static void *next(void *instance, struct c_utils_position *pos);

static void *prev(void *instance, struct c_utils_position *pos);

static bool append(void *instance, struct c_utils_position *pos, void *item);

static bool prepend(void *instance, struct c_utils_position *pos, void *item);




struct c_utils_list *c_utils_list_create() {
	struct c_utils_list_conf conf = {};
	return c_utils_list_create_conf(&conf);
}

struct c_utils_list *c_utils_list_create_conf(struct c_utils_list_conf *conf) {
	if(!conf)
		return NULL;

	struct c_utils_list *list;

	if(conf->ref_counted)
		list = c_utils_ref_create(sizeof(*list), destroy_list);
	else
		list = calloc(1, sizeof(*list));

	if(!list)
		return NULL;

	list->head = list->tail = NULL;
	list->size = 0;
	list->is_sorted = 1;

	if (conf->concurrent)
		list->lock = c_utils_scoped_lock_rwlock(NULL, conf->logger);
	else
		list->lock = c_utils_scoped_lock_no_op();

	if (!list->lock) {
		C_UTILS_LOG_ERROR(conf->logger, "Was unable to create scoped_lock for rwlock!");
		free(list);
		return NULL;
	}

	list->conf = *conf;

	return list;
}

struct c_utils_list *c_utils_list_from(void *array, size_t size) {
	if(!array)
		return NULL;

	struct c_utils_list_conf conf = {};
	return c_utils_list_from_conf(array, size, &conf);
}

struct c_utils_list *c_utils_list_from_conf(void *array, size_t size, struct c_utils_list_conf *conf) {
	struct c_utils_list *list = c_utils_list_create_conf(conf);
	if(list) {
		int i = 0;
		for (;i<size;i++)
			c_utils_list_add(list, ((void **) array)[i]);
	}

	return list;
}

void c_utils_list_remove_all(struct c_utils_list *list) {
	if(!list)
		return;
	
	// Acquire Writer Lock
	C_UTILS_SCOPED_LOCK0(list->lock)
		delete_all_nodes(list, NULL);
}

void c_utils_list_delete_all(struct c_utils_list *list) {
	if(!list)
		return;
	
	// Acquire Writer Lock
	C_UTILS_SCOPED_LOCK0(list->lock) 
		delete_all_nodes(list, list->conf.del);
}

void c_utils_list_destroy(struct c_utils_list *list) {
	if(!list)
		return;

	if(list->conf.ref_counted) {
		c_utils_ref_dec(list);
		return;
	}

	destroy_list(list);
}

bool c_utils_list_add(struct c_utils_list *list, void *item) {
	if(!list)
		return false;

	if(!item) {
		C_UTILS_LOG_ERROR(list->conf.logger, "This list does not support NULL elements!");
		return false;
	}
	
	struct c_utils_node *node;
	C_UTILS_ON_BAD_MALLOC(node, list->conf.logger, sizeof(*node))
		return false;

	node->item = item;
	
	// Acquire Writer Lock
	C_UTILS_SCOPED_LOCK0(list->lock) {
		if (!list->size)
			return add_as_only(list, node);

		if (list->conf.cmp)
			return add_sorted(list, node, list->conf.cmp);
		else
			return add_unsorted(list, node);
	} // Release Writer Lock

	C_UTILS_UNACCESSIBLE;
}

void c_utils_list_remove(struct c_utils_list *list, void *item) {
	if(!list)
		return;

	if(!item) {
		C_UTILS_LOG_ERROR(list->conf.logger, "This list does not support NULL elements!");
		return;
	}

	remove_item(list, item, false);
}

void c_utils_list_delete(struct c_utils_list *list, void *item) {
	if(!list)
		return;

	if(!item) {
		C_UTILS_LOG_ERROR(list->conf.logger, "This list does not support NULL elements!");
		return;
	}

	remove_item(list, item, true);
}

void *c_utils_list_remove_at(struct c_utils_list *list, unsigned int index) {
	if(!list)
		return NULL;

	return remove_at(list, index, false);
}

void c_utils_list_delete_at(struct c_utils_list *list, unsigned int index) {
	if(!list)
		return;

	remove_at(list, index, true);
}

size_t c_utils_list_size(struct c_utils_list *list) {
	return list->size;
}

bool c_utils_list_for_each(struct c_utils_list *list, void (*callback)(void *item)) {
	if(!list)
		return false;

	if(!callback) {
		C_UTILS_LOG_ERROR(list->conf.logger, "A callback function is expected to be invoked on each item!");
		return false;
	}

	// Acquire Reader Lock
	C_UTILS_SCOPED_LOCK1(list->lock)
		for_each_item(list, callback);
		
	return true;
}

bool c_utils_list_sort(struct c_utils_list *list) {
	if(!list)
		return false;

	if(!list->conf.cmp) {
		C_UTILS_LOG_ERROR(list->conf.logger, "A comparator was not passed with initial configuration!");
		return false;
	}

	// Acquire Writer Lock
	C_UTILS_SCOPED_LOCK0(list->lock)
		insertion_sort_list(list, list->conf.cmp);

	return true;
}

bool c_utils_list_contains(struct c_utils_list *list, void *item) {
	if(!list)
		return false;

	if(!item) {
		C_UTILS_LOG_ERROR(list->conf.logger, "This list does not support NULL elements!");
		return false;
	}

	// Acquire Reader Lock
	C_UTILS_SCOPED_LOCK1(list->lock)
		return !!item_to_node(list, item);

	C_UTILS_UNACCESSIBLE;
}

void *c_utils_list_get(struct c_utils_list *list, unsigned int index) {
	if(!list)
		return NULL;

	// Acquire Reader Lock
	C_UTILS_SCOPED_LOCK1(list->lock) {
		struct c_utils_node *node = index_to_node(list, index);
		return node ? node->item : NULL;
	} // Release Reader Lock

	C_UTILS_UNACCESSIBLE;
}

void **c_utils_list_as_array(struct c_utils_list *list, size_t *size) {
	if(!list)
		return NULL;

	// Acquire Reader Lock
	C_UTILS_SCOPED_LOCK1(list->lock) {
		void **array_of_items;
		C_UTILS_ON_BAD_MALLOC(array_of_items, list->conf.logger, sizeof(void *) * list->size)
			return NULL;

		int index = 0;
		for (struct c_utils_node *node = list->head; node; node = node->_double.next)
			array_of_items[index++] = node->item;

		*size = index;
		return array_of_items;
	} // Release Reader Lock

	C_UTILS_UNACCESSIBLE;
}

struct c_utils_iterator *c_utils_list_iterator(struct c_utils_list *list) {
	if(!list)
		return NULL;

	struct c_utils_iterator *it;
	C_UTILS_ON_BAD_CALLOC(it, list->conf.logger, sizeof(*it))
		return NULL;

	it->handle = list;
	it->head = head;
	it->tail = tail;
	it->next = next;
	it->prev = prev;
	it->append = append;
	it->prepend = prepend;

	// Increment reference count for iterator.
	if(list->conf.ref_counted) {
		c_utils_ref_inc(list);
		it->conf.ref_counted = true;
	}

	return it;
}




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
		c_utils_list_sort(list);

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
	C_UTILS_LOG_ERROR(list->conf.logger, "Was unable to add an item, sortedly, to the list!\n");
	
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

static void *remove_at(struct c_utils_list *list, unsigned int index, bool delete_item) {
	// Acquire Writer Lock
	C_UTILS_SCOPED_LOCK0(list->lock) {
		struct c_utils_node *temp_node = index_to_node(list, index);
		
		if (temp_node) {
			void *item = temp_node->item;
			remove_node(list, temp_node, delete_item ? list->conf.del : NULL);
			return item;
		} else {
			C_UTILS_LOG_WARNING(list->conf.logger, "The node returned from Index_To_Node was NULL!\n");
			return NULL;
		}
	} // Release Writer Lock

	C_UTILS_UNACCESSIBLE;
}

static void remove_item(struct c_utils_list *list, void *item, bool delete_item) {
	// Acquire Writer Lock
	C_UTILS_SCOPED_LOCK0(list->lock) {
		struct c_utils_node *node = item_to_node(list, item);

		remove_node(list, node, delete_item ? list->conf.del : NULL);
	} // Release Writer Lock
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




static int delete_all_nodes(struct c_utils_list *list, c_utils_delete_cb del) {
	while (list->head)
		remove_node(list, list->head, del);
	
	return 1;
}

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
		while ((node = node->_double.prev) && --i != index)
			;
		C_UTILS_ASSERT(i == index, list->conf.logger, "Error in Node Traversal!Expected index %u, stopped at index %d!", index, i);
		return node;
	} else {
		int i = 0;
		node = list->head;
		while ((node = node->_double.next) && ++i != index);
		C_UTILS_ASSERT(i == index, list->conf.logger, "Error in Node Traversal!Expected index %u, stopped at index %d!", index, i);
	}
	
	return node;
}

static void for_each_item(struct c_utils_list *list, void (*callback)(void *item)) {
	struct c_utils_node *node = NULL;
	for (node = list->head; node; node = node->_double.next)
		callback(node->item);
}

static void destroy_list(void *instance) {
	struct c_utils_list *list = instance;
	delete_all_nodes(list, list->conf.del_items_on_free ? list->conf.del : NULL);

	c_utils_scoped_lock_destroy(list->lock);
	free(list);
}



static const int curr_valid = 1 << 0;
static const int prev_valid = 1 << 1;
static const int next_valid = 1 << 2;

static unsigned int is_valid(struct c_utils_list *list, struct c_utils_position *pos) {
	unsigned int retval = 0;
	struct c_utils_node *tmp = NULL;
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
	C_UTILS_SCOPED_LOCK1(list->lock) {
		head = list->head;
		update_pos(pos, head);
		return get_item(head);
	} // Release Reader Lock

	C_UTILS_UNACCESSIBLE;
}

static void *tail(void *instance, struct c_utils_position *pos) {
	struct c_utils_list *list = instance;
	struct c_utils_node *tail;

	// Acquire Reader Lock
	C_UTILS_SCOPED_LOCK1(list->lock) {
		tail = list->tail;
		update_pos(pos, tail);
		return get_item(tail);
	} // Release Reader Lock

	C_UTILS_UNACCESSIBLE;
}

static void *next(void *instance, struct c_utils_position *pos) {
	struct c_utils_list *list = instance;
	struct c_utils_node *next = NULL;

	// Acquire Reader Lock
	C_UTILS_SCOPED_LOCK1(list->lock) {
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

	C_UTILS_UNACCESSIBLE;
}

static void *prev(void *instance, struct c_utils_position *pos) {
	struct c_utils_list *list = instance;
	struct c_utils_node *prev;
	
	// Acquire Reader Lock
	C_UTILS_SCOPED_LOCK1(list->lock) {
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

	C_UTILS_UNACCESSIBLE;
}

static bool append(void *instance, struct c_utils_position *pos, void *item) {
	struct c_utils_list *list = instance;
	
	struct c_utils_node *node = malloc(sizeof(*node));
	if (!node) {
		C_UTILS_LOG_ASSERT(list->conf.logger, "malloc: '%s'", strerror(errno));
		return false;
	}
	node->item = item;
	
	// Acquire Writer Lock
	C_UTILS_SCOPED_LOCK0(list->lock) {
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

	C_UTILS_UNACCESSIBLE;
}

static bool prepend(void *instance, struct c_utils_position *pos, void *item) {
	struct c_utils_list *list = instance;
	
	struct c_utils_node *node;
	C_UTILS_ON_BAD_MALLOC(node, list->conf.logger, sizeof(*node))
		return false;
	node->item = item;
	
	// Acquire Writer Lock
	C_UTILS_SCOPED_LOCK0(list->lock) {
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

	C_UTILS_UNACCESSIBLE;
}