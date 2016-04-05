#ifndef C_UTILS_TREE_H
#define C_UTILS_TREE_H

#include <stddef.h>

#include "../io/logger.h"

#define C_UTILS_TREE_RC_INSTANCE 1 << 0

#define C_UTILS_TREE_RC_ITEM 1 << 1

#define C_UTILS_TREE_CONCURRENT 1 << 2

#define C_UTILS_TREE_

struct c_utils_tree;

struct c_utils_tree_conf {
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
	struct c_utils_logger *logger;
};

struct c_utils_tree *c_utils_tree_create(int (*comparator)(const void *, const void *));

struct c_utils_tree *c_utils_tree_create_conf(int (*comparator)(const void *, const void *), struct c_utils_tree_conf *conf);

bool c_utils_tree_insert(struct c_utils_tree *tree, void *item);

size_t c_utils_tree_size(struct c_utils_tree *tree);

void *c_utils_tree_get(struct c_utils_tree *tree);

void *c_utils_tree_remove(struct c_utils_tree *tree);

void c_utils_tree_remove_all(struct c_utils_tree *tree);

bool c_utils_tree_delete(struct c_utils_tree *tree);

void c_utils_tree_delete_all(struct c_utils_tree *tree);

void c_utils_tree_destroy(struct c_utils_tree *tree);

#endif /* C_UTILS_TREE_H */