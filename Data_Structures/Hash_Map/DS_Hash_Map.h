#include <DS_Helpers.h>
#include <MU_Logger.h>
#include <MU_Cond_Locks.h>

typedef struct {
	size_t amount_of_buckets;
	size_t max_size;
} DS_Hash_Map_t;

/// Create a hash map with the requested amount of buckets, the bounds if applicable, and whether to initialize and use rwlocks.
DS_Hash_Map_t *DS_Hash_Map_create(size_t buckets, size_t max_size, unsigned char init_locks);

/// Add a key-value pair to a hash map, and if the comparator is not NULL, it will be a hash-set operation.
int DS_Hash_Map_add(DS_Hash_Map_t *map, const char *key, const void *value, DS_comparator_cb cmp);

/// Obtains the value from the key provided.
void *DS_Hash_Map_get(DS_Hash_Map_t *map, const char *key);

/// Return and remove the value at the key provided, deleting it if the deletion callback is not NULL.
void *DS_Hash_Map_remove(DS_Hash_Map_t *map, const char *key, DS_delete_cb del);

/// Determines whether or not the key exists within the map.
int DS_Hash_Map_contains_key(DS_Hash_Map_t *map, const char *key);

/// Determines whether or not the item exists within the map. If the comparator is NULL, it is a point-comparison, otherwise it will be based om cmp.
int DS_Hash_Map_contains_value(DS_Hash_Map_t *map, const void *item, DS_comparator_cb cmp);

/// Uses said callback on all elements inside of the map based on the general callback supplied.
int DS_Hasp_Map_for_each(DS_Hash_Map_t *map, DS_general_cb cb);

/// Will clear the map of all elements, calling the deletion callback on each element if it is not NULL.
int DS_Hash_Map_clear(DS_Hash_Map_t *map, DS_delete_cb del);

/// Determines whether or not the hash map is full.
int DS_Hash_Map_is_empty(DS_Hash_Map_t *map);

/// Determines whether or not the hash map is full. If there is no max size, it will always be true.
int DS_Hash_Map_is_full(DS_Hash_Map_t *map);

/// Determines the size at the time this function is called.
size_t DS_Hash_Map_size(DS_Hash_Map_t *map);