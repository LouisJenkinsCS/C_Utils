#ifndef C_UTILS_HEAP_H
#define C_UTILS_HEAP_H

#include <stddef.h>

#include "../io/logger.h"

#define C_UTILS_HEAP_RC_INSTANCE 1 << 0

#define C_UTILS_HEAP_RC_ITEM 1 << 1

#define C_UTILS_HEAP_CONCURRENT 1 << 2

#define C_UTILS_HEAP_DELETE_ON_DESTROY 1 << 3

struct c_utils_heap;

struct c_utils_heap_conf {
	int flags;
	struct {
		struct {
			void (*item)(void *);
		} destructors;
	} callbacks;
	struct {
		size_t initial;
		size_t max;
	} size;
	struct {
		float rate;
		float trigger;
	} growth;
	struct c_utils_logger *logger;
};

#ifdef C_UTILS_NO_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_heap heap_t;
typedef struct c_utils_heap_conf heap_conf_t;

/*
	Macros
*/
#define HEAP_RC_INSTANCE C_UTILS_HEAP_RC_INSTANCE
#define HEAP_RC_ITEM C_UTILS_HEAP_RC_ITEM
#define HEAP_CONCURRENT C_UTILS_HEAP_CONCURRENT
#define HEAP_DELETE_ON_DESTROY C_UTILS_HEAP_DELETE_ON_DESTROY

/*
	Functions
*/
#define heap_create(...) c_utils_heap_create(__VA_ARGS__)
#define heap_create_conf(...) c_utils_heap_create_conf(__VA_ARGS__)
#define heap_create_from(...) c_utils_heap_create_from(__VA_ARGS__)
#define heap_create_from_conf(...) c_utils_heap_create_from_conf(__VA_ARGS__)
#define heap_insert(...) c_utils_heap_insert(__VA_ARGS__)
#define heap_size(...) c_utils_heap_size(__VA_ARGS__)
#define heap_get(...) c_utils_heap_get(__VA_ARGS__)
#define heap_remove(...) c_utils_heap_remove(__VA_ARGS__)
#define heap_remove_all(...) c_utils_heap_remove_all(__VA_ARGS__)
#define heap_delete(...) c_utils_heap_delete(__VA_ARGS__)
#define heap_delete_all(...) c_utils_heap_delete_all(__VA_ARGS__)
#define heap_destroy(...) c_utils_heap_destroy(__VA_ARGS__)
#endif

/*
	Creates a simple binary heap with order depending on the comparator passed. It is not concurrent nor reference counted.
*/
struct c_utils_heap *c_utils_heap_create(int (*comparator)(const void *, const void *));

/*
	Creates a configurable binary heap, specific to the passed configuration data.
*/
struct c_utils_heap *c_utils_heap_create_conf(int (*comparator)(const void *, const void *), struct c_utils_heap_conf *conf);

/*
	Creates a simple binary heap from the passed array with a complexity of O(N).
*/
struct c_utils_heap *c_utils_heap_create_from(int (*comparator)(const void *, const void *), void **arr, size_t len);

/*
	Create a configurable binary heap from the passed array with a complexity of O(N).
*/
struct c_utils_heap *c_utils_heap_create_from_conf(int (*comparator)(const void *, const void *), void **arr, size_t len, struct c_utils_heap_conf *conf);

/*
	Inserts a new element into the heap, with a complexity of O(log(N)). If the heap is full (or unable to resize) it will return false.
*/
bool c_utils_heap_insert(struct c_utils_heap *tree, void *item);

/*
	Obtains the number of elements inside of the heap.
*/
size_t c_utils_heap_size(struct c_utils_heap *tree);

/*
	Obtains the max (according to comparator) element in the heap. If it is reference counted, the reference count is incremented, hence it is up to the
	caller to release their reference count to the data returned. O(1)
*/
void *c_utils_heap_get(struct c_utils_heap *tree);

/*
	Returns and removes the data from the heap. If it is reference counted, the reference count is not incremented, as it is transfered to the caller.
	O(log(N))
*/
void *c_utils_heap_remove(struct c_utils_heap *tree);

/*
	Removes all elements from the heap. If they are reference counted, it merely removes it's reference over each item.
*/
void c_utils_heap_remove_all(struct c_utils_heap *tree);

/*
	Deletes the element from the heap. If it is reference counted, it will remove it's reference to the list. If not, it will invoke it's destructor.
*/
bool c_utils_heap_delete(struct c_utils_heap *tree);

/*
	Deletes all elements in the heap. If they are reference counted, it merely removes it's reference, otherwise it will invoke it's destructor on each.
*/
void c_utils_heap_delete_all(struct c_utils_heap *tree);

/*
	Destroys the underlying heap. If the heap is reference counted, it merely decrements the count over it (hence C_UTILS_REF_DEC can be called instead)
*/
void c_utils_heap_destroy(struct c_utils_heap *tree);

#endif /* C_UTILS_HEAP_H */