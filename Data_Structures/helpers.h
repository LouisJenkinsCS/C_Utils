#ifndef DS_HELPERS_H
#define DS_HELPERS_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "io/logger.h"

struct c_utils_node {
	/// Union allows multiple types of nodes with only one type.
	union {
		struct {
			struct DS_Node_t *next;
		} _single;
		struct {
			struct DS_Node_t *next;
			struct DS_Node_t *prev;
		} _double;
	};
	void *item;
};

DS_Node_t *DS_Node_create(void *item, struct c_utils_logger *logger) {
	DS_Node_t *node = calloc(1, sizeof(*node));
	if(!node){
		C_UTILS_LOG_ASSERT(logger, "calloc: '%s'", strerror(errno));
		return NULL;
	}
	node->item = item;
	return node;
}
 
typedef void (*c_utils_general_cb)(void *);

typedef int (*c_utils_comparator_cb)(const void *, const void *);

typedef char *(*c_utils_to_string_cb)(const void *);

typedef void (*c_utils_delete_cb)(void *);

#endif /* END DS_HELPERS_H */