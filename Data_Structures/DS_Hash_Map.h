#include <DS_Helpers.h>

#ifdef DS_HASH_MAP_KEY_MAX_SIZE
#define DS_HASH_MAP_KEY_SIZE DS_HASH_MAP_KEY_MAX_SIZE
#else
#define DS_HASH_MAP_KEY_SIZE 128
#endif

typedef struct DS_Bucket_t {
	/// The key associated with each bucket.
	char key[DS_HASH_MAP_KEY_SIZE + 1];
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
	/// RWLock to enforce thread-safety.
	pthread_rwlock_t *lock;
} DS_Hash_Map_t;

/**
 * 
 * @param amount_of_buckets
 * @param init_locks
 * @return 
 */
DS_Hash_Map_t *DS_Hash_Map_create(size_t amount_of_buckets, unsigned char init_locks);

/**
 * 
 * @param map
 * @param key
 * @param value
 * @return 
 */
bool DS_Hash_Map_add(DS_Hash_Map_t *map, char *key, void *value);

/**
 * 
 * @param map
 * @param key
 * @return 
 */
void *DS_Hash_Map_get(DS_Hash_Map_t *map, const char *key);

/**
 * 
 * @param map
 * @param key
 * @param del
 * @return 
 */
void *DS_Hash_Map_remove(DS_Hash_Map_t *map, const char *key, DS_delete_cb del);

/**
 * 
 * @param map
 * @param value
 * @param cmp
 * @return 
 */
const char *DS_Hash_Map_contains(DS_Hash_Map_t *map, const void *value, DS_comparator_cb cmp);

/**
 * 
 * @param map
 * @param key_prefix
 * @param delimiter
 * @param val_suffix
 * @param size
 * @param to_string
 * @return 
 */
char **DS_Hash_Map_key_value_to_string(DS_Hash_Map_t *map, const char *key_prefix, const char *delimiter, const char *val_suffix, size_t *size, DS_to_string_cb to_string);

/**
 * 
 * @param map
 * @param cb
 * @return 
 */
bool DS_Hasp_Map_for_each(DS_Hash_Map_t *map, DS_general_cb cb);

/**
 * 
 * @param map
 * @param del
 * @return 
 */
bool DS_Hash_Map_clear(DS_Hash_Map_t *map, DS_delete_cb del);

/**
 * 
 * @param map
 * @return 
 */
size_t DS_Hash_Map_size(DS_Hash_Map_t *map);

/**
 * 
 * @param map
 * @param del
 * @return 
 */
bool DS_Hash_Map_destroy(DS_Hash_Map_t *map, DS_delete_cb del);