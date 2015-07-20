#ifndef DS_HELPERS_H
#define DS_HELPERS_H

#include <stdint.h>
#include <errno.h>

typedef void (*DS_general_cb)(void *);

typedef int (*DS_comparator_cb)(const void *, const void *);

typedef void (*DS_delete_cb)(void *);

#endif /* END DS_HELPERS_H */