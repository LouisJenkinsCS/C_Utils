#include "map.h"

#include "../misc/alloc_check.h"
#include "../misc/argument_check.h"
#include "../io/logger.h"
#include "../threading/scoped_lock.h"

struct c_utils_bucket {
	/// The key associated with each bucket.
	char key[C_UTILS_HASH_MAP_KEY_SIZE + 1];
	/// The value associated with each key.
	void *value;
	/// Determines whether or not the bucket is in use, to allow for "caching".
	volatile unsigned char in_use;
	/// In case of collision, it will chain to the next. There is no limit.
	struct c_utils_bucket *next;
};

struct c_utils_map {
	/// Array of all bucket head nodes.
	struct c_utils_bucket **buckets;
	/// The size of the hash map.
	size_t size;
	/// Maximum amount of buckets.
	size_t amount_of_buckets;
	/// RWLock to enforce thread-safety.
	struct c_utils_scoped_lock *lock;
	/// Configuration
	struct c_utils_map_conf conf;
};

/// Very simple and straight forward hash function. Bob Jenkin's hash. Default hash if none supplied.
static uint32_t hash_key(const void *key) {
	const char *k = key;
	uint32_t hash = 0;

	for (uint32_t i = 0;i < strlen(k); ++i) {
		hash += k[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

static void delete_bucket(struct c_utils_bucket *bucket, c_utils_delete_cb del) {
	if (del)
		del(bucket->value);
	free(bucket);
}

static void delete_all_buckets(struct c_utils_bucket **buckets, size_t amount_of_buckets, c_utils_delete_cb del) {
	size_t i = 0;
	struct c_utils_bucket *curr_bucket = NULL, *head_bucket = NULL;
	for (; i < amount_of_buckets; i++) {
		if (!(head_bucket = curr_bucket = buckets[i]))
			continue;

		while ((head_bucket = curr_bucket->next)) {
			delete_bucket(curr_bucket, del);
			curr_bucket = head_bucket;
		}

		if (curr_bucket)
			delete_bucket(curr_bucket, del);
	}
}

static void for_each_bucket(struct c_utils_bucket *bucket, c_utils_general_cb callback, unsigned char done_with_bucket) {
	if (!bucket) return;

	do {
		if (!bucket->in_use) continue;
		
		callback(bucket->value);
		
		if (done_with_bucket)
			bucket->in_use = 0;
	} while ((bucket = bucket->next));
}

/// Non threaded version of clearing a map.
static int clear_map(struct c_utils_map *map, c_utils_delete_cb del) {
	size_t i = 0, total_buckets = map->amount_of_buckets;
	for (; i < total_buckets; i++)
		for_each_bucket(map->buckets[i], del, 1);
	
	map->size = 0;
	return 1;
}

static void *get_value_from_bucket(struct c_utils_bucket *bucket, const char *key) {
	if (!bucket) 
		return NULL;

	do {
		if (!bucket->in_use)
			continue;
		
		if (strncmp(bucket->key, key, C_UTILS_HASH_MAP_KEY_SIZE) == 0)
			break;
	} while ((bucket = bucket->next));
	
	return bucket ? bucket->value : NULL;
}

static size_t get_bucket_index(const char *key, size_t amount_of_buckets) {
	return hash_key(key) % amount_of_buckets;;
}

static int bucket_is_valid(struct c_utils_bucket *bucket) {
	return bucket && bucket->in_use;
}

static struct c_utils_bucket *get_bucket(struct c_utils_bucket **buckets, size_t amount_of_buckets, const char *key) {
	return buckets[get_bucket_index(key, amount_of_buckets)];
}

static char *get_key_if_match(struct c_utils_bucket *bucket, const void *value, c_utils_comparator_cb comparator) {
	if (!bucket) return NULL;
	
	do {
		if (!bucket->in_use) continue;
		
		if (comparator ? comparator(bucket->value, value) == 0 : bucket->value == value)
			break;
	} while ((bucket = bucket->next));
	
	return bucket ? bucket->key : NULL;
}

static struct c_utils_bucket *create_bucket(char *key, void *value, struct c_utils_bucket *next) {
	struct c_utils_bucket *bucket;
	C_UTILS_ON_BAD_CALLOC(bucket, logger, sizeof(*bucket))
		return NULL;

	snprintf(bucket->key, C_UTILS_HASH_MAP_KEY_SIZE + 1, "%s", key);
	bucket->value = value;
	bucket->next = next;
	
	return bucket;
}

/// Create a hash map with the requested amount of buckets, the bounds if applicable, and whether to initialize and use rwlocks.
struct c_utils_map *c_utils_map_create(size_t amount_of_buckets, bool synchronized) {
	struct c_utils_map *map;
	C_UTILS_ON_BAD_CALLOC(map, logger, sizeof(*map))
		goto err;
	
	map->amount_of_buckets = amount_of_buckets ? amount_of_buckets : 1;
	
	C_UTILS_ON_BAD_CALLOC(map->buckets, logger, sizeof(*map->buckets) * amount_of_buckets)
		goto err_buckets;
	
	map->lock = synchronized ? c_utils_scoped_lock_rwlock(NULL, logger) : c_utils_scoped_lock_no_op();
	if(!map->lock) {
		C_UTILS_LOG_ERROR(logger, "Was unable to create the scoped_lock!");
		goto err_lock;
	}

	return map;

	err_lock:
		free(map->buckets);
	err_buckets:
		free(map);
	err:
		return NULL;
}

/// Add a key-value pair to a hash map.
bool c_utils_map_add(struct c_utils_map *map, char *key, void *value) {
	C_UTILS_ARG_CHECK(logger, false, map, key);

	C_UTILS_SCOPED_LOCK0(map->lock) {
		char trunc_key[C_UTILS_HASH_MAP_KEY_SIZE + 1];
		snprintf(trunc_key, C_UTILS_HASH_MAP_KEY_SIZE + 1, "%s", key);

		size_t index = get_bucket_index(trunc_key, map->amount_of_buckets);
		struct c_utils_bucket *bucket = map->buckets[index];
		if (!bucket) {
			bucket = (map->buckets[index] = create_bucket(trunc_key, value, NULL));
			if (!bucket) {
				C_UTILS_LOG_ERROR(logger, "c_utils_map_add->create_bucket: \"Was unable to create a bucket!\"");
				return false;
			}
			bucket->in_use = 1;
			
			map->size++;
			return true;
		}
		void *key_exists = get_value_from_bucket(bucket, trunc_key);
		if (key_exists)
			return false;

		do {
			if (!bucket->in_use) {
				sprintf(bucket->key, "%s", trunc_key);
				bucket->value = value;
				bucket->in_use = 1;

				map->size++;
				break;
			} else if (!bucket->next) {
				bucket->next = create_bucket(trunc_key, value, NULL);
				if(!bucket->next)
					return false;
				bucket->next->in_use = 1;
				
				map->size++;
				break;
			}
		} while ((bucket = bucket->next));
	} // Release writer lock.

	return true;
}

/// Obtains the value from the key provided.
void *c_utils_map_get(struct c_utils_map *map, const char *key) {
	C_UTILS_ARG_CHECK(logger, NULL, map, map && map->size, map && map->buckets, key);
	
	C_UTILS_SCOPED_LOCK1(map->lock) {
		char trunc_key[C_UTILS_HASH_MAP_KEY_SIZE + 1];
		snprintf(trunc_key, C_UTILS_HASH_MAP_KEY_SIZE + 1, "%s", key);

		struct c_utils_bucket *bucket = get_bucket(map->buckets, map->amount_of_buckets, trunc_key);
		return bucket_is_valid(bucket) ? get_value_from_bucket(bucket, key) : NULL;
	} // Release reader lock.

	C_UTILS_UNACCESSIBLE;
}

/// Return and remove the value at the key provided, deleting it if the deletion callback is not NULL.
void *c_utils_map_remove(struct c_utils_map *map, const char *key, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, NULL, map, map && map->buckets, map && map->size, key);

	C_UTILS_SCOPED_LOCK0(map->lock) {
		char trunc_key[C_UTILS_HASH_MAP_KEY_SIZE + 1];
		snprintf(trunc_key, C_UTILS_HASH_MAP_KEY_SIZE + 1, "%s", key);

		struct c_utils_bucket *bucket = get_bucket(map->buckets, map->amount_of_buckets, trunc_key);
		if (!bucket_is_valid(bucket))
			return NULL;

		void *item = get_value_from_bucket(bucket, trunc_key);
		bucket->in_use = 0;
		map->size--;
		
		return item;
	} // Release writer lock.

	C_UTILS_UNACCESSIBLE;
}

/// Determines whether or not the item exists within the map. If the comparator is NULL, it is a pointer-comparison, otherwise it will be based om cmp.
const char *c_utils_map_contains(struct c_utils_map *map, const void *value, c_utils_comparator_cb cmp) {
	C_UTILS_ARG_CHECK(logger, NULL, map, map && map->buckets, map && map->size);

	C_UTILS_SCOPED_LOCK1(map->lock) {
		size_t i = 0, total_buckets = map->amount_of_buckets;
		// O(N) complexity.
		for (; i < total_buckets; i++) {
			char *key = NULL;
			struct c_utils_bucket *bucket = map->buckets[i];
			if ((key = get_key_if_match(bucket, value, cmp)))
				return key;
		}
	} // Release reader lock.

	return NULL;
}

/// Uses said callback on all elements inside of the map based on the general callback supplied.
bool c_utils_map_for_each(struct c_utils_map *map, c_utils_general_cb callback_function) {
	C_UTILS_ARG_CHECK(logger, false, map, map && map->buckets, map && map->size, callback_function);

	C_UTILS_SCOPED_LOCK1(map->lock) {
		size_t i = 0, total_buckets = map->amount_of_buckets;
		for (; i < total_buckets; i++)
			for_each_bucket(map->buckets[i], callback_function, 0);
	} // Release reader lock.
	return true;
}

/// Will clear the map of all elements, calling the deletion callback on each element if it is not NULL.
bool c_utils_map_clear(struct c_utils_map *map, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, false, map, map && map->buckets, map && map->size);
	
	C_UTILS_SCOPED_LOCK0(map->lock) 
		return clear_map(map, del);

	C_UTILS_UNACCESSIBLE;
}

/// Determines the size at the time this function is called.
size_t c_utils_map_size(struct c_utils_map *map) {
	C_UTILS_ARG_CHECK(logger, 0, map, map && map->buckets, map && map->size);

	C_UTILS_SCOPED_LOCK1(map->lock) 
		return map->size;

	C_UTILS_UNACCESSIBLE;
}

char **c_utils_map_key_value_to_string(struct c_utils_map *map, const char *key_prefix, const char *delimiter, const char *val_suffix, size_t *size, c_utils_to_string_cb to_string) {
	C_UTILS_ARG_CHECK(logger, (*size = 0, NULL), map, size);

	/*
		Note here: We wrap the goto inside of the scoped_lock block, because if it were outside of it,
		all variables would be removed from the stack and would leak the original array. Since the goto
		and still in-scope, there should not be an issue.
	*/
	const size_t buf_size = 256;
	C_UTILS_SCOPED_LOCK1(map->lock) {
		char **arr = NULL;
		
		size_t arr_size = map->size, str_allocated = 0;
		if (!arr_size) {
			C_UTILS_LOG_WARNING(logger, "The hash map is empty!");
			goto error;
		}

		arr = malloc(sizeof(char *) * arr_size);
		if (!arr) {
			C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
			goto error;
		}

		size_t i = 0;
		for (; i < map->amount_of_buckets; i++) {
			struct c_utils_bucket *bucket = map->buckets[i];
			if (!bucket) 
				continue;
			
			do {
				if (!bucket->in_use) 
					continue;
				
				char *buf = malloc(buf_size);
				if (!buf) {
					C_UTILS_LOG_ASSERT(logger, "malloc: '%s'", strerror(errno));
					goto error;
				}
				
				snprintf(buf, buf_size + 1, "%s%s%s%s%s", key_prefix ? key_prefix : "", bucket->key,
					delimiter ? delimiter : "", to_string ? to_string(bucket->value) : (char *)bucket->value, val_suffix ? val_suffix : "");
				
				arr[str_allocated++] = buf;
			} while ((bucket = bucket->next));
		}
		
		*size = str_allocated;
		return arr;

		error:
			if (arr) {
				size_t i = 0;
				for (; i < str_allocated; i++)
					free(arr[i]);
				free(arr);
			}
			*size = 0;
			return NULL;
	} // Release reader lock

	C_UTILS_UNACCESSIBLE;
}

bool c_utils_map_destroy(struct c_utils_map *map, c_utils_delete_cb del) {
	C_UTILS_ARG_CHECK(logger, false, map);

	C_UTILS_SCOPED_LOCK0(map->lock) {
		delete_all_buckets(map->buckets, map->amount_of_buckets, del);

		map->amount_of_buckets = 0;
		map->size = 0;
		map->buckets = NULL;
	} // Release writer lock.

	c_utils_scoped_lock_destroy(map->lock);
	free(map);

	return true;
}
