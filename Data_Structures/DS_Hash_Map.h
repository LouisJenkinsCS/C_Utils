#include <DS_Helpers.h>

#ifdef C_UTILS_USE_POSIX_STD
#define hash_map_t DS_Hash_Map_t
#define hash_map_create(...) DS_Hash_Map_create(__VA_ARGS__)
#define hash_map_add(...) DS_Hash_Map_add(__VA_ARGS__)
#define hash_map_get(...) DS_Hash_Map_get(__VA_ARGS__)
#define hash_map_remove(...) DS_Hash_Map_remove(__VA_ARGS__)
#define hash_map_contains(...) DS_Hash_Map_contains(__VA_ARGS__)
#define hash_map_clear(...) DS_Hash_Map_clear(__VA_ARGS__)
#define hash_map_size(...) DS_Hash_Map_size(__VA_ARGS__)
#define hash_map_destroy(...) DS_Hash_Map_destroy(__VA_ARGS__)
#define hash_map_key_value_to_string(...) DS_Hash_Map_key_value_to_string(__VA_ARGS__)
#define hash_map_for_each(...) DS_Hash_Map_for_each(__VA_ARGS__)
#endif

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
 * Creates a hash map with the initial amount of buckets. The hash map does not resize, so care should
 * be taken when declaring amount of buckets. If the hash map is full, it will chain it. The map is
 * synchronized if init_locks is non 0.
 * @param amount_of_buckets Amount of buckets.
 * @param init_locks Synchronized?
 * @return Map instance, or NULL if amount_of_buckets is 0, or an allocation error occurs.
 */
DS_Hash_Map_t *DS_Hash_Map_create(size_t amount_of_buckets, unsigned char init_locks);

/**
 * Adds the item with the given key to the hash map as a new key-value pair.
 * @param map Instance.
 * @param key Key.
 * @param value Value.
 * @return True if successful, false is allocation error, key, map, or value is NULL
 */
bool DS_Hash_Map_add(DS_Hash_Map_t *map, char *key, void *value);

/**
 * Obtains the item from the map through it's key. 
 * @param map Instance.
 * @param key Key
 * @return Item found, or NULL if not found.
 */
void *DS_Hash_Map_get(DS_Hash_Map_t *map, const char *key);

/**
 * Removes the item from the map, optionally deleting it by invoking del on it if not null.
 * @param map Instance.
 * @param key Key.
 * @param del Deletion callback.
 * @return The item removed, NULL if deleted or not present.
 */
void *DS_Hash_Map_remove(DS_Hash_Map_t *map, const char *key, DS_delete_cb del);

/**
 * Checks if the hash map contains the given item, using the comparator to check. If the comparator, cmp,
 * is left null, it will only compare the pointer addresses themselves.
 * @param map Instance.
 * @param value Item to look for.
 * @param cmp Comparator.
 * @return The key which is associated with it, or null if not found.
 */
const char *DS_Hash_Map_contains(DS_Hash_Map_t *map, const void *value, DS_comparator_cb cmp);

/**
 * Calls the to_string callback on each item in the map, obtaining it's stringified representation. Then, it will
 * apply key_prefix before each key, a delimiter after each key, and val_suffix after each value. As this returns
 * an array of strings, one string for each key-value pair, size must be a valid pointer to a size_t variable, as
 * it is used to return the size of the array.
 * @param map Instance
 * @param key_prefix Prefix ("" if left null)
 * @param delimiter Delimiter ("" if left null)
 * @param val_suffix Suffix ("" if left null)
 * @param size Pointer used to return array size.
 * @param to_string Callback to turn each value into a string.
 * @return An array of strings for each key-value pair, with it's size returned through size.
 */
char **DS_Hash_Map_key_value_to_string(DS_Hash_Map_t *map, const char *key_prefix, const char *delimiter, const char *val_suffix, size_t *size, DS_to_string_cb to_string);

/**
 * Calls cb on each item in the list.
 * @param map Instance
 * @param cb Callback
 * @return True if map and cb are not null.
 */
bool DS_Hasp_Map_for_each(DS_Hash_Map_t *map, DS_general_cb cb);

/**
 * Clears the hash map of all items, calling del on each if declared.
 * @param map Instance.
 * @param del Deletion callback.
 * @return True if map is not null.
 */
bool DS_Hash_Map_clear(DS_Hash_Map_t *map, DS_delete_cb del);

/**
 * Returns the amount of items contained in this Hash Map
 * @param map Instance
 * @return Number of items.
 */
size_t DS_Hash_Map_size(DS_Hash_Map_t *map);

/**
 * Destroys the hash map, optionally calling the passed deletion callback on
 * each item in the list if declared.
 * @param map Hash Map instance
 * @param del Deletion callback, called on each item.
 * @return True if map is not null.
 */
bool DS_Hash_Map_destroy(DS_Hash_Map_t *map, DS_delete_cb del);