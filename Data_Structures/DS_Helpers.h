#ifndef DS_HELPERS_H
#define DS_HELPERS_H

void (*DS_general_cb)(const void **);

int (*DS_comparator_cb)(const void *, const void *);

int (*DS_delete_cb)(void *);

#endif /* END DS_HELPERS_H */