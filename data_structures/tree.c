#include "tree.h"

struct c_utils_tree {
	void **heap;
	int (*cmp)(const void *, const void *);
	size_t size;
	size_t used;
	struct c_utils_tree_conf conf;
};

static size_t default_initial = 64;

static size_t default_max = 1024;

static size_t left(size_t parent);

static size_t right(size_t parent);

static size_t parent(size_t child);

static void heapify_up(size_t index);

static void heapify_down(size_t index);

struct c_utils_tree *c_utils_tree_create(int (*comparator)(const void *, const void *)) {
	struct c_utils_tree_conf conf = {};
	return c_utils_tree_conf(comparator, &conf);
}

struct c_utils_tree *c_utils_tree_create_conf(int (*comparator)(const void *, const void *), struct c_utils_tree_conf *conf) {
	if(!comparator || !conf)
		return NULL;


}

bool c_utils_tree_insert(struct c_utils_tree *tree, void *item);

size_t c_utils_tree_size(struct c_utils_tree *tree);

void *c_utils_tree_get(struct c_utils_tree *tree);

void *c_utils_tree_remove(struct c_utils_tree *tree);

void c_utils_tree_remove_all(struct c_utils_tree *tree);

bool c_utils_tree_delete(struct c_utils_tree *tree);

void c_utils_tree_delete_all(struct c_utils_tree *tree);

void c_utils_tree_destroy(struct c_utils_tree *tree);