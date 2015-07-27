#ifndef DS_HELPERS_H
#define DS_HELPERS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <MU_Logger.h>
#include <MU_Cond_Locks.h>
#include <MU_Arg_Check.h>

typedef void (*DS_general_cb)(void *);

typedef int (*DS_comparator_cb)(const void *, const void *);

typedef char *(*DS_to_string_cb)(const void *);

typedef void (*DS_delete_cb)(void *);

#endif /* END DS_HELPERS_H */