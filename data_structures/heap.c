#include "heap.h"
#include "../threading/scoped_lock.h"
#include "../memory/ref_count.h"
#include "../misc/alloc_check.h"

struct c_utils_heap {
	void **data;
	int (*cmp)(const void *, const void *);
	size_t size;
	size_t used;
	struct c_utils_scoped_lock *lock;
	struct c_utils_heap_conf conf;
};

static size_t default_initial = 64;

static size_t default_max = 1024;

static double default_growth_rate = 2;

static double default_growth_trigger = .75;

static size_t left(size_t parent);

static size_t right(size_t parent);

static size_t parent(size_t child);

static void swap(struct c_utils_heap *heap, size_t first, size_t second);

static void *extract_max(struct c_utils_heap *heap);

static void heapify_up(struct c_utils_heap *heap);

static void heapify_down(struct c_utils_heap *heap);

static void shift_down(struct c_utils_heap *heap, size_t index);

static bool resize(struct c_utils_heap *heap, size_t size);

static void destroy_heap(void *heap);

static void configure(struct c_utils_heap_conf *conf);


struct c_utils_heap *c_utils_heap_create(int (*comparator)(const void *, const void *)) {
	struct c_utils_heap_conf conf = {};
	return c_utils_heap_create_conf(comparator, &conf);
}

struct c_utils_heap *c_utils_heap_create_from(int (*comparator)(const void *, const void *), void **arr, size_t len) {
	struct c_utils_heap_conf conf = {};
	return c_utils_heap_create_from_conf(comparator, arr, len, &conf);
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

	if(!heap) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed during creation of the heap!");
		goto err;
	}

	if(conf->flags & C_UTILS_HEAP_CONCURRENT)
		heap->lock = c_utils_scoped_lock_mutex(NULL, NULL);
	else
		heap->lock = c_utils_scoped_lock_no_op();

	if(!heap->lock) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed during creation of the scoped lock!");
		goto err_lock;
	}

	heap->data = malloc(sizeof(void *) * conf->size.initial);
	if(!heap->data) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed during creation of the heap container!");
		goto err_heap;
	}

	heap->size = conf->size.initial;
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

struct c_utils_heap *c_utils_heap_create_from_conf(int (*comparator)(const void *, const void *), void **arr, size_t len, struct c_utils_heap_conf *conf) {
	if(!comparator || !conf)
		return NULL;

	if(!arr || !len) {
		C_UTILS_LOG_TRACE(conf->logger, "NULL array or length of 0 passed, creating a standard heap!");
		return c_utils_heap_create_conf(comparator, conf);
	}

	if(len > conf->size.initial)
		conf->size.initial = len;

	if(len > conf->size.max)
		conf->size.max = len;

	struct c_utils_heap *heap = c_utils_heap_create_conf(comparator, conf);
	if(!heap) {
		C_UTILS_LOG_ERROR(conf->logger, "Failed during creation of base heap!");
		return NULL;
	}

	for(size_t i = 0; i < len; i++)
		heap->data[i] = arr[i];

	for(size_t i = len / 2; i >= 1; i--) {
		shift_down(heap, i);
	}

	return heap;
}

bool c_utils_heap_insert(struct c_utils_heap *heap, void *item) {
	if(!heap)
		return false;

	if(!item) {
		C_UTILS_LOG_WARNING(heap->conf.logger, "This heap does not support NULL values!");
		return false;
	}

	C_UTILS_SCOPED_LOCK(heap->lock) {
		heap->data[++heap->used] = item;

		if(heap->conf.flags & C_UTILS_HEAP_RC_ITEM)
			C_UTILS_REF_INC(item);

		/*
			After inserting an item into the heap, we must move the recently
			added value to it's correct place if necessary.
		*/
		heapify_up(heap);

		if(((double)heap->used / heap->size) > heap->conf.growth.trigger)
			resize(heap, heap->size * heap->conf.growth.rate);
	}

	return true;
}

size_t c_utils_heap_size(struct c_utils_heap *heap) {
	if(!heap)
		return 0;
	
	return heap->used;
}

void *c_utils_heap_get(struct c_utils_heap *heap) {
	if(!heap)
		return NULL;

	C_UTILS_SCOPED_LOCK(heap->lock) {
		if(!heap->used)
			return NULL;

		void *item = heap->data[1];
		if(heap->conf.flags & C_UTILS_HEAP_RC_ITEM)
			C_UTILS_REF_INC(item);

		return item;
	}

	C_UTILS_UNACCESSIBLE;
}

void *c_utils_heap_remove(struct c_utils_heap *heap) {
	if(!heap)
		return NULL;

	C_UTILS_SCOPED_LOCK(heap->lock) {
		if(!heap->used)
			return NULL;

		return extract_max(heap);
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_heap_remove_all(struct c_utils_heap *heap) {
	if(!heap)
		return;

	C_UTILS_SCOPED_LOCK(heap->lock) {
		if(!heap->used)
			return;

		for(size_t i = 1; i <= heap->used; i++) {
			void *item = heap->data[i];
			if(heap->conf.flags & C_UTILS_HEAP_RC_ITEM)
				C_UTILS_REF_DEC(item);
		}

		heap->used = 0;
	}
}

bool c_utils_heap_delete(struct c_utils_heap *heap) {
	if(!heap)
		return false;

	C_UTILS_SCOPED_LOCK(heap->lock) {
		if(!heap->used)
			return false;

		void *item = extract_max(heap);

		if(heap->conf.flags & C_UTILS_HEAP_RC_ITEM)
			C_UTILS_REF_DEC(item);
		else
			heap->conf.callbacks.destructors.item(heap);

		return true;
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_heap_delete_all(struct c_utils_heap *heap) {
	if(!heap)
		return;

	C_UTILS_SCOPED_LOCK(heap->lock) {
		if(!heap->used)
			return;

		for(size_t i = 1; i <= heap->used; i++) {
			void *item = heap->data[i];
			if(heap->conf.flags & C_UTILS_HEAP_RC_ITEM)
				C_UTILS_REF_DEC(item);
			else
				heap->conf.callbacks.destructors.item(heap);
		}

		heap->used = 0;
	}
}

void c_utils_heap_destroy(struct c_utils_heap *heap) {
	if(!heap)
		return;

	if(heap->conf.flags & C_UTILS_HEAP_RC_INSTANCE) {
		C_UTILS_REF_DEC(heap);
		return;
	}

	destroy_heap(heap);
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

static void swap(struct c_utils_heap *heap, size_t first, size_t second) {
	void *item = heap->data[first];
	heap->data[first] = heap->data[second];
	heap->data[second] = item;
}

static void *extract_max(struct c_utils_heap *heap) {
	void *item = heap->data[1];
	heap->data[1] = heap->data[heap->used--];

	heapify_down(heap);

	return item;
}

static void heapify_up(struct c_utils_heap *heap) {
	for(size_t i = heap->used; i > 1; i = parent(i)) {
		if(heap->cmp(heap->data[i], heap->data[parent(i)]) > 0)
			swap(heap, i, parent(i));
		else
			break;
	}
}

static void heapify_down(struct c_utils_heap *heap) {
	// If the root node is less than it's children, correct it here.
	for(size_t i = 1; i <= heap->used;) {
		size_t largest = i;

		if(left(i) <= heap->used && heap->cmp(heap->data[left(i)], heap->data[largest]) > 0)
			largest = left(i);
		
		if(right(i) <= heap->used && heap->cmp(heap->data[right(i)], heap->data[largest]) > 0)
			largest = right(i);

		// No change?
		if(largest == i)
			break;

		swap(heap, largest, i);
		i = largest;
	}
}

static void shift_down(struct c_utils_heap *heap, size_t index) {
	for(size_t i = index; i <= heap->used;) {
		if(heap->cmp(heap->data[left(i)], heap->data[i]) > 0) {
			swap(heap, left(i), i);
			i = left(i);
		} else if(heap->cmp(heap->data[right(i)], heap->data[i]) > 0) {
			swap(heap, right(i), i);
			i = right(i);
		} else {
			break;
		}
	}
}

static bool resize(struct c_utils_heap *heap, size_t size) {
	if(heap->size == heap->conf.size.max)
		return false;

	size_t new_size = (heap->conf.size.max < size) ? heap->conf.size.max : size;
	C_UTILS_ON_BAD_REALLOC(&heap->data, heap->conf.logger, sizeof(void *) * new_size)
		return false;

	heap->size = new_size;
	return true;
}

static void destroy_heap(void *instance) {
	struct c_utils_heap *heap = instance;
	if(!heap)
		return;

	if(heap->conf.flags & C_UTILS_HEAP_DELETE_ON_DESTROY)
		c_utils_heap_delete_all(heap);
	else
		c_utils_heap_remove_all(heap);

	c_utils_scoped_lock_destroy(heap->lock);

	free(heap->data);
}

static void configure(struct c_utils_heap_conf *conf) {
	if(!conf->growth.rate)
		conf->growth.rate = default_growth_rate;

	if(!conf->growth.trigger)
		conf->growth.trigger = default_growth_trigger;

	if(!conf->size.initial)
		conf->size.initial = default_initial;

	if(!conf->size.max)
		conf->size.max = default_max;

	if(!conf->callbacks.destructors.item)
		conf->callbacks.destructors.item = free;
}