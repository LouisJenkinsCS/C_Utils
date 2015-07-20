#include <DS_Helpers.h>
#include <MU_Logger.h>
#include <string.h>
#include <MU_Cond_Locks.h>

typedef struct DS_Bucket_t {
	/// The key associated with each bucket.
	char *key;
	/// The value associated with each key.
	void *value;
	/// Determines whether or not the bucket is in use, to allow for "caching".
	volatile unsigned char in_use;
	/// In case of collision, it will chain to the next. There is no limit.
	struct DS_Bucket_t *next;
} DS_Bucket_t;

typedef struct {
	/// Array of all bucket head nodes.
	DS_Bucket_t **buckets;
	/// The size of the hash map.
	size_t size;
	/// Maximum amount of buckets.
	size_t amount_of_buckets;
} DS_Hash_Map_t;

/// Create a hash map with the requested amount of buckets, the bounds if applicable, and whether to initialize and use rwlocks.
DS_Hash_Map_t *DS_Hash_Map_create(size_t amount_of_buckets, unsigned char init_locks);

/// Add a key-value pair to a hash map, and if the comparator is not NULL, it will be a hash-set operation.
int DS_Hash_Map_add(DS_Hash_Map_t *map, char *key, void *value);

/// Obtains the value from the key provided.
void *DS_Hash_Map_get(DS_Hash_Map_t *map, const char *key);

/// Return and remove the value at the key provided, deleting it if the deletion callback is not NULL.
void *DS_Hash_Map_remove(DS_Hash_Map_t *map, const char *key, DS_delete_cb del);

/// Determines whether or not the item exists within the map. If the comparator is NULL, it is a pointer-comparison, otherwise it will be based om cmp.
const char *DS_Hash_Map_contains(DS_Hash_Map_t *map, const void *value, DS_comparator_cb cmp);

/// Uses said callback on all elements inside of the map based on the general callback supplied.
int DS_Hasp_Map_for_each(DS_Hash_Map_t *map, DS_general_cb cb);

/// Will clear the map of all elements, calling the deletion callback on each element if it is not NULL.
int DS_Hash_Map_clear(DS_Hash_Map_t *map, DS_delete_cb del);

/// Determines the size at the time this function is called.
size_t DS_Hash_Map_size(DS_Hash_Map_t *map);

int DS_Hash_Map_destroy(DS_Hash_Map_t *map, DS_delete_cb del);