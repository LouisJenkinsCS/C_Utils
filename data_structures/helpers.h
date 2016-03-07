#ifndef DS_HELPERS_H
#define DS_HELPERS_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../io/logger.h"
#include "../misc/alloc_check.h"

struct c_utils_node {
	/// Union allows multiple types of nodes with only one type.
	union {
		struct {
			struct c_utils_node *next;
		} _single;
		struct {
			struct c_utils_node *next;
			struct c_utils_node *prev;
		} _double;
	};
	void *item;
};


typedef void (*c_utils_general_cb)(void *);

typedef int (*c_utils_comparator_cb)(const void *, const void *);

typedef char *(*c_utils_to_string_cb)(const void *);

typedef void (*c_utils_delete_cb)(void *);

#endif /* END DS_HELPERS_H */
