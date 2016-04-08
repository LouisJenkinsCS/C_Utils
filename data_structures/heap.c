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

static size_t left(size_t parent);

static size_t right(size_t parent);

static size_t parent(size_t child);

static void swap(struct c_utils_heap *tree, size_t first, size_t second);

static void *extract_max(struct c_utils_heap *tree);

static void heapify_up(struct c_utils_heap *tree);

static void heapify_down(struct c_utils_heap *tree);

static bool resize(struct c_utils_heap *tree, size_t size);

static void destroy_heap(struct c_utils_heap *tree);

static void configure(struct c_utils_heap_conf *conf);


struct c_utils_heap *c_utils_heap_create(int (*comparator)(const void *, const void *)) {
	struct c_utils_heap_conf conf = {};
	return c_utils_heap_create_conf(comparator, &conf);
}

struct c_utils_heap *c_utils_heap_create_conf(int (*comparator)(const void *, const void *), struct c_utils_heap_conf *conf) {
	if(!comparator || !conf)
		return NULL;

	configure(conf);

	struct c_utils_heap *heap;

	if(conf->flags & C_UTILS_HEAP_RC_INSTANCE) {
		struct c_utils_ref_count_conf rc_conf =
		{ 
			.logger = conf->logger,
			.destructor = destroy_heap
		};

		heap = c_utils_ref_create_conf(sizeof(*heap), &rc_conf);
	} else {
		heap = malloc(sizeof(*heap));
	}

	if(!heap)
		C_UTILS_LOG_ERROR(conf->logger, "Failed during creation of the heap!");
		goto err;

	if(conf->flags & C_UTILS_HEAP_CONCURRENT)
		heap->lock = c_utils_scoped_lock_mutex(NULL, NULL);
	else
		heap->lock = c_utils_scoped_lock_no_op();

	if(!heap->lock) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed during creation of the scoped lock!");
		goto err_lock;
	}

	heap->heap = malloc(sizeof(void *) * conf->size.initial);
	if(!heap->heap) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed during creation of the heap container!");
		goto err_heap;
	}

	heap->cmp = comparator;
	heap->conf = *conf;

	return heap;

	err_heap:
		c_utils_scoped_lock_destroy(heap->lock);
	err_lock:
		if(conf->flags & C_UTILS_HEAP_RC_INSTANCE)
			c_utils_ref_destroy(heap);
		else
			free(heap);
	err:
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

		return extract_max(tree);
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_heap_remove_all(struct c_utils_heap *tree) {
	if(!tree)
		return NULL;

	C_UTILS_SCOPED_LOCK(tree->lock) {
		if(!tree->used)
			return NULL;

		void *item = extract_max(tree);
		
		if(tree->conf.flags & C_UTILS_HEAP_RC_ITEM)
			C_UTILS_REF_DEC(item);
	}

	C_UTILS_UNACCESSIBLE;
}

bool c_utils_heap_delete(struct c_utils_heap *tree) {
	if(!tree)
		return false;

	C_UTILS_SCOPED_LOCK(tree->lock) {
		if(!tree->used)
			return false;

		void *item = extract_max(tree);

		if(tree->conf.flags & C_UTILS_HEAP_RC_ITEM)
			C_UTILS_REF_DEC(item);
		else
			tree->conf.callbacks.destructors.item(tree);

		return true;
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_heap_delete_all(struct c_utils_heap *tree) {
	if(!tree)
		return NULL;

	C_UTILS_SCOPED_LOCK(tree->lock) {
		if(!tree->used)
			return NULL;

		void *item = extract_max(tree);
		
		if(tree->conf.flags & C_UTILS_HEAP_RC_ITEM)
			C_UTILS_REF_DEC(item);
		else
			tree->conf.callbacks.destructors.item(tree);
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_heap_destroy(struct c_utils_heap *tree) {
	if(!tree)
		return;

	if(tree->conf.flags & C_UTILS_HEAP_RC_INSTANCE) {
		C_UTILS_REF_DEC(tree);
		return;
	}

	destroy_heap(tree);
}



static size_t left(size_t parent) {
	return parent * 2;
}

static size_t right(size_t parent) {
	return parent * 2 + 1;
}

static size_t parent(size_t child) {
	return child / 2;
}

static void swap(struct c_utils_heap *tree, size_t first, size_t second) {
	void *item = tree->heap[first];
	tree->heap[first] = tree->heap[second];
	tree->heap[second] = item;
}

static void *extract_max(struct c_utils_heap *tree) {

}

static void heapify_up(struct c_utils_heap *tree) {
	for(size_t i = tree->used - 1; i > 1; i = parent(i)) {
		if(tree->cmp(tree->heap[i], tree->heap[parent(i)]) > 0)
			swap(tree, i, parent(i));
		else
			break;
	}
}

static void heapify_down(struct c_utils_heap *tree) {

}