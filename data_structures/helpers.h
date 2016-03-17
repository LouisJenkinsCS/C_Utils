#ifndef DS_HELPERS_H
#define DS_HELPERS_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>

#include "../io/logger.h"
#include "../misc/alloc_check.h"

struct c_utils_node {
	// Next node if used.
	struct c_utils_node *next;
	// Previous node if used
	struct c_utils_node *prev;
	// If this node is valid, used by iterator implementations.
	volatile bool is_valid;
	// Item assocaited with node.
	void *item;
};


typedef void (*c_utils_general_cb)(void *);

typedef int (*c_utils_comparator_cb)(const void *, const void *);

typedef char *(*c_utils_to_string_cb)(const void *);

typedef void (*c_utils_delete_cb)(void *);

#endif /* END DS_HELPERS_H */
