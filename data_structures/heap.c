#include "heap.h"
#include "../threading/scoped_lock.h"
#include "../memory/ref_count.h"

struct c_utils_heap {
	void **heap;
	int (*cmp)(const void *, const void *);
	size_t size;
	size_t used;
	struct c_utils_scoped_lock *lock;
	struct c_utils_heap_conf conf;
};

static size_t default_initial = 64;

static size_t default_max = 1024;

static size_t left(struct c_utils_heap *tree, size_t parent);

static size_t right(struct c_utils_heap *tree, size_t parent);

static size_t parent(struct c_utils_heap *tree, size_t child);

static void heapify_up(struct c_utils_heap *tree);

static void heapify_down(struct c_utils_heap *tree);

static bool resize(struct c_utils_heap *tree, size_t size);

struct c_utils_heap *c_utils_heap_create(int (*comparator)(const void *, const void *)) {
	struct c_utils_heap_conf conf = {};
	return c_utils_heap_create_conf(comparator, &conf);
}

struct c_utils_heap *c_utils_heap_create_conf(int (*comparator)(const void *, const void *), struct c_utils_heap_conf *conf) {
	if(!comparator || !conf)
		return NULL;


}

bool c_utils_heap_insert(struct c_utils_heap *tree, void *item) {
	if(!tree)
		return false;

	if(!item) {
		C_UTILS_LOG_WARNING(tree->conf.logger, "This tree does not support NULL values!");
		return false;
	}

	C_UTILS_SCOPED_LOCK(tree->lock) {
		if(!tree->used)
			tree->used++;

		tree->heap[tree->used++] = item;

		if(tree->conf.flags & C_UTILS_HEAP_RC_ITEM)
			C_UTILS_REF_INC(item);

		/*
			After inserting an item into the tree, we must move the recently
			added value to it's correct place if necessary.
		*/
		heapify_up(tree);

		if(((double)tree->used / tree->size) > tree->conf.growth.trigger)
			resize(tree, tree->size * tree->conf.growth.rate);
	}
}

size_t c_utils_heap_size(struct c_utils_heap *tree) {
	if(!tree)
		return 0;
	
	return tree->used;
}

void *c_utils_heap_get(struct c_utils_heap *tree) {
	if(!tree)
		return NULL;

	C_UTILS_SCOPED_LOCK(tree->lock) {
		if(!tree->used)
			return NULL;

		void *item = tree->heap[1];
		if(tree->conf.flags & C_UTILS_HEAP_RC_ITEM)
			C_UTILS_REF_INC(item);

		return item;
	}

	C_UTILS_UNACCESSIBLE;
}

void *c_utils_heap_remove(struct c_utils_heap *tree) {
	if(!tree)
		return NULL;

	C_UTILS_SCOPED_LOCK(tree->lock) {
		if(!tree->used)
			return NULL;

		void *item = tree->heap[1];
		// TODO: Pickup here

		return item;
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_heap_remove_all(struct c_utils_heap *tree);

bool c_utils_heap_delete(struct c_utils_heap *tree);

void c_utils_heap_delete_all(struct c_utils_heap *tree);

void c_utils_heap_destroy(struct c_utils_heap *tree);