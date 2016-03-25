#include "helpers.h"
#include "../io/logger.h"

#include <stdbool.h>

/*
	A simple hash map implementation which can be made thread-safe by passing true for synchronized on construction.

	Synchronicity is handled through a rwlock, allowing for multiple concurrent reader access, and is useful for a
	Read-Often Write-Rarely map. The hash map as of current does not support resizing, but it does support infinite chaining, 
	hence knowing the amount_of_buckets before hand is very important.
*/
struct c_utils_map;

#define C_UTILS_MAP_FOR_EACH_KEY(key, map)

#define C_UTILS_MAP_FOR_EACH_VALUE(value, map)

#define C_UTILS_MAP_FOR_EACH_PAIR(key, value, map)

#define C_UTILS_MAP_CONCURRENT 1 << 0
#define C_UTILS_MAP_RC_INSTANCE 1 << 1
#define C_UTILS_MAP_RC_KEY 1 << 2
#define C_UTILS_MAP_RC_VALUE 1 << 3
#define C_UTILS_MAP_DELETE_ON_DESTROY 1 << 4
#define C_UTILS_MAP_SHRINK_ON_TRIGGER 1 << 5

/*
	concurrent:
		default:
			false
		note:
			Will initialize and utilize a reader-writer lock to allow efficient and well-optimized concurrent access.
			This way it will allow any iterators or map functions that do not mutate the list to proceed concurrently
			in an efficient manor. If this is not specified, the map will not use a lock and hence any concurrent access
			will yield undefined behavior. The default is good for if you do not need concurrent access.
	num_buckets:
		default:
			64
		note:
			Sets the initial amount of buckets to prevent incessant resizing. Great for if you know exactly how many items the
			map will hold, or can at least guess at such.
	ref_counted:
		default:
			false
		note:
			If this data structure is reference counted. This will automatically increment the count of this instance when an iterator is created,
			and decremenet the count when an iterator is destroyed. The map can be destroyed either through REF_DEC or map_destroy
	del_on_free:
		default:
			false
		note:
			Will destroy each key and value with their associated destructors if specified if flagged as true.
	obj_len:
		default:
			strlen(obj)
		note:
			If specified, then the default hash can be used with a pre-defined length safely to allow easier hashing. If this is
			not specified, the default behavior will be to apply strlen on the object to obtain it's length.
	hash_fnc:
		default:
			default_hash(obj, obj_len)
		note:
			If specified, the custom user-defined hash will be used to hash the object, which will be the Bob Jenkins' hash, up to the passed
			length, or strlen if not defined. This works fine for majority of structures, but a more specified one may be more desired.
	key_destructor:
		default:
			NULL
		note:
			If specified, and if del_on_free is specified, then each key will also be destroyed on destruction of the map. Note that this will not
			be called when delete is called because the key must be used to obtain the value.
	value_destructor:
		default:
			NULL
		note:
			If specified, it will be used on delete calls, and if del_on_free is specified, it will be used on each value in the map as it is destroyed.
	value_cmp:
		default:
			NULL
		note:
			If specified, it will be used to determined if the value specified is passed. If it is not specified, but obj_len IS specified, we use memcmp to
			determine if it is the same. If neither is, we compare the memory addresses.
	logger:
		default:
			NULL
		note:
			If specified, any and all trace logs and warnings/errors are logged to it. Otherwise they are silently ignored.
*/
struct c_utils_map_conf {
	/// Configuration flags
	int flags;
	/// Default number of buckets
	size_t num_buckets;
	/// Callbacks
	struct {
		/// Destructors
		struct {
			/// Key destructor
			void (*key)(void *);
			/// Value destructor
			void (*value)(void *);
		} destructors;
		/// Comparators used for deep searching.
		struct {
			/// Key comparator
			int (*key)(const void *, const void *);
			/// Value comparator
			int (*value)(const void *, const void *);
		} comparators;
		/// Hash function
		uint32_t (*hash_function)(const void *key, size_t len);
	} callbacks;
	struct {
		/// What ratio should we grow at?
		double ratio;
		/// When should we trigger?
		double trigger;
	} growth;
	struct {
		/// If so, at what rate?
		double ratio;
		/// And when?
		double trigger;
	} shrink;
	/// The size of the key being hashed. Should always be specified if not a string.
	size_t key_len;
	/// The size of the values being added. Should be specified if plan on searching without specifying value comparator.
	size_t value_len;
	/// Logger
	struct c_utils_logger *logger;
};

/*
	Example of configuration object...

	map_conf_t conf =
	{
		.flags = MAP_CONCURRENT | MAP_RC_INSTANCE | MAP_SHRINK_ON_TRIGGER | MAP_DELETE_ON_DESTROY,
		.num_buckets = 128,
		.callbacks = 
		{
			.destructors = 
			{
				.key = free,
				.value = destroy_value
			},
			.hash_function = my_custom_hash,
			.value_comparator = my_custom_comparator
		},
		.growth = 
		{
			.ratio = 1.5,
			.trigger = .75
		},
		.shrink = 
		{
			.ratio = .75,
			.trigger = .1
		},
		.obj_len = sizeof(struct my_obj),
		.logger = my_logger
	};
*/

#ifdef NO_C_UTILS_PREFIX
/*
	Typedefs
*/
typedef struct c_utils_map map_t;

/*
	Functions
*/
#define map_create(...) c_utils_map_create(__VA_ARGS__)
#define map_create_conf(...) c_utils_map_create_conf(__VA_ARGS__)
#define map_add(...) c_utils_map_add(__VA_ARGS__)
#define map_get(...) c_utils_map_get(__VA_ARGS__)
#define map_remove(...) c_utils_map_remove(__VA_ARGS__)
#define map_remove_all(...) c_utils_map_remove_all(__VA_ARGS__)
#define map_delete(...) c_utils_map_delete(__VA_ARGS__)
#define map_delete_all(...) c_utils_map_delete_all(__VA_ARGS__)
#define map_contains(...) c_utils_map_contains(__VA_ARGS__)
#define map_clear(...) c_utils_map_clear(__VA_ARGS__)
#define map_size(...) c_utils_map_size(__VA_ARGS__)
#define map_destroy(...) c_utils_map_destroy(__VA_ARGS__)
#define map_key_value_to_string(...) c_utils_map_key_value_to_string(__VA_ARGS__)
#define map_for_each(...) c_utils_map_for_each(__VA_ARGS__)
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
struct c_utils_map *c_utils_map_create();

struct c_utils_map *c_utils_map_create_conf(struct c_utils_map_conf *conf);

/**
 * Adds the item with the given key to the hash map as a new key-value pair. 
 *
 * Writer-Lock: Not Concurrent, Is Thread Safe.
 * @param map Instance.
 * @param key Key.
 * @param value Value.
 * @return True if successful, false is allocation error, key, map, or value is NULL
 */
bool c_utils_map_add(struct c_utils_map *map, void *key, void *value);

/**
 * Obtains the item from the map through it's key.
 *
 * Reader-Lock: Concurrent operation.
 * @param map Instance.
 * @param key Key
 * @return Item found, or NULL if not found.
 */
void *c_utils_map_get(struct c_utils_map *map, const void *key);

/**
 * Removes the item from the map, optionally deleting it by invoking del on it if not null.
 *
 * Writer-Lock: Not Concurrent, Is Thread Safe.
 * @param map Instance.
 * @param key Key.
 * @param del Deletion callback.
 * @return The item removed, NULL if deleted or not present.
 */
void *c_utils_map_remove(struct c_utils_map *map, const void *key);

void c_utils_map_remove_all(struct c_utils_map *map);

bool c_utils_map_delete(struct c_utils_map *map, const void *key);

void c_utils_map_delete_all(struct c_utils_map *map);

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
const void *c_utils_map_contains(struct c_utils_map *map, const void *value);

/**
 * Calls cb on each item in the list.
 *
 * Reader-Lock: Concurrent operation.
 * @param map Instance
 * @param cb Callback
 * @return True if map and cb are not null.
 */
bool c_utils_map_for_each(struct c_utils_map *map, void (*callback)(const void *key, const void *value));

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
void c_utils_map_destroy(struct c_utils_map *map);
