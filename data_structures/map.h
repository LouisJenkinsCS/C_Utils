#include "helpers.h"

#include <stdbool.h>

/*
	A simple hash map implementation which can be made thread-safe by passing true for synchronized on construction.

	Synchronicity is handled through a rwlock, allowing for multiple concurrent reader access, and is useful for a
	Read-Often Write-Rarely map. The hash map as of current does not support resizing, but it does support infinite chaining, 
	hence knowing the amount_of_buckets before hand is very important.
*/
struct c_utils_map;

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_map map_t;

/*
	Functions
*/
#define map_create(...) c_utils_map_create(__VA_ARGS__)
#define map_add(...) c_utils_map_add(__VA_ARGS__)
#define map_get(...) c_utils_map_get(__VA_ARGS__)
#define map_remove(...) c_utils_map_remove(__VA_ARGS__)
#define map_contains(...) c_utils_map_contains(__VA_ARGS__)
#define map_clear(...) c_utils_map_clear(__VA_ARGS__)
#define map_size(...) c_utils_map_size(__VA_ARGS__)
#define map_destroy(...) c_utils_map_destroy(__VA_ARGS__)
#define map_key_value_to_string(...) c_utils_map_key_value_to_string(__VA_ARGS__)
#define map_for_each(...) c_utils_map_for_each(__VA_ARGS__)
#endif

#ifdef C_UTILS_HASH_MAP_KEY_MAX_SIZE
#define C_UTILS_HASH_MAP_KEY_SIZE C_UTILS_HASH_MAP_KEY_MAX_SIZE
#else
#define C_UTILS_HASH_MAP_KEY_SIZE 128
#endif

/**
 * Creates a hash map with the initial amount of buckets. The hash map does not resize, so care should
 * be taken when declaring amount of buckets. If the hash map is full, it will chain it. The map is
 * synchronized if init_locks is non 0.
 *
 * Reader-Lock: Concurrent operation.
 * @param amount_of_buckets Amount of buckets.
 * @param synchronized Synchronized?
 * @return Map instance, or NULL if amount_of_buckets is 0, or an allocation error occurs.
 */
struct c_utils_map *c_utils_map_create(size_t amount_of_buckets, bool synchronized);

/**
 * Adds the item with the given key to the hash map as a new key-value pair. 
 *
 * Writer-Lock: Not Concurrent, Is Thread Safe.
 * @param map Instance.
 * @param key Key.
 * @param value Value.
 * @return True if successful, false is allocation error, key, map, or value is NULL
 */
bool c_utils_map_add(struct c_utils_map *map, char *key, void *value);

/**
 * Obtains the item from the map through it's key.
 *
 * Reader-Lock: Concurrent operation.
 * @param map Instance.
 * @param key Key
 * @return Item found, or NULL if not found.
 */
void *c_utils_map_get(struct c_utils_map *map, const char *key);

/**
 * Removes the item from the map, optionally deleting it by invoking del on it if not null.
 *
 * Writer-Lock: Not Concurrent, Is Thread Safe.
 * @param map Instance.
 * @param key Key.
 * @param del Deletion callback.
 * @return The item removed, NULL if deleted or not present.
 */
void *c_utils_map_remove(struct c_utils_map *map, const char *key, c_utils_delete_cb del);

/**
 * Checks if the hash map contains the given item, using the comparator to check. If the comparator, cmp,
 * is left null, it will only compare the pointer addresses themselves.
 *
 * Reader-Lock: Concurrent operation.
 * @param map Instance.
 * @param value Item to look for.
 * @param cmp Comparator.
 * @return The key which is associated with it, or null if not found.
 */
const char *c_utils_map_contains(struct c_utils_map *map, const void *value, c_utils_comparator_cb cmp);

/**
 * Calls the to_string callback on each item in the map, obtaining it's stringified representation. Then, it will
 * apply key_prefix before each key, a delimiter after each key, and val_suffix after each value. As this returns
 * an array of strings, one string for each key-value pair, size must be a valid pointer to a size_t variable, as
 * it is used to return the size of the array.
 *
 * Reader-Lock: Concurrent operation.
 * @param map Instance
 * @param key_prefix Prefix ("" if left null)
 * @param delimiter Delimiter ("" if left null)
 * @param val_suffix Suffix ("" if left null)
 * @param size Pointer used to return array size.
 * @param to_string Callback to turn each value into a string.
 * @return An array of strings for each key-value pair, with it's size returned through size.
 */
char **c_utils_map_key_value_to_string(struct c_utils_map *map, const char *key_prefix, const char *delimiter, const char *val_suffix, size_t *size, c_utils_to_string_cb to_string);

/**
 * Calls cb on each item in the list.
 *
 * Reader-Lock: Concurrent operation.
 * @param map Instance
 * @param cb Callback
 * @return True if map and cb are not null.
 */
bool c_utils_map_for_each(struct c_utils_map *map, c_utils_general_cb cb);

/**
 * Clears the hash map of all items, calling del on each if declared.
 *
 * Writer-Lock: Not Concurrent, Is Thread Safe.
 * @param map Instance.
 * @param del Deletion callback.
 * @return True if map is not null.
 */
bool c_utils_map_clear(struct c_utils_map *map, c_utils_delete_cb del);

/**
 * Returns the amount of items contained in this Hash Map
 *
 * Reader-Lock: Concurrent operation.
 * @param map Instance
 * @return Number of items.
 */
size_t c_utils_map_size(struct c_utils_map *map);

/**
 * Destroys the hash map, optionally calling the passed deletion callback on
 * each item in the list if declared.
 * @param map Hash Map instance
 * @param del Deletion callback, called on each item.
 * @return True if map is not null.
 */
bool c_utils_map_destroy(struct c_utils_map *map, c_utils_delete_cb del);
