#ifndef C_UTILS_HEAP_H
#define C_UTILS_HEAP_H

#include <stddef.h>

#include "../io/logger.h"

#define C_UTILS_HEAP_RC_INSTANCE 1 << 0

#define C_UTILS_HEAP_RC_ITEM 1 << 1

#define C_UTILS_HEAP_CONCURRENT 1 << 2

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

struct c_utils_heap *c_utils_heap_create(int (*comparator)(const void *, const void *));

struct c_utils_heap *c_utils_heap_create_conf(int (*comparator)(const void *, const void *), struct c_utils_heap_conf *conf);

bool c_utils_heap_insert(struct c_utils_heap *tree, void *item);

size_t c_utils_heap_size(struct c_utils_heap *tree);

void *c_utils_heap_get(struct c_utils_heap *tree);

void *c_utils_heap_remove(struct c_utils_heap *tree);

void c_utils_heap_remove_all(struct c_utils_heap *tree);

bool c_utils_heap_delete(struct c_utils_heap *tree);

void c_utils_heap_delete_all(struct c_utils_heap *tree);

void c_utils_heap_destroy(struct c_utils_heap *tree);

#endif /* C_UTILS_HEAP_H */