#include "map.h"

#include "../misc/alloc_check.h"
#include "../misc/argument_check.h"
#include "../io/logger.h"
#include "../threading/scoped_lock.h"
#include "../memory/ref_count.h"

struct c_utils_bucket {
	/// Key
	void *key;
	/// Value
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

static const int default_buckets = 64;
static const double default_growth_rate = 2;
static const double default_growth_trigger = .5;
static const double default_shrink_rate = .5;
static const double default_shrink_trigger = .1;

/// Very simple and straight forward hash function. Bob Jenkin's hash. Default hash if none supplied.
static uint32_t hash_key(const void *key, size_t len) {
	const char *k = key;
	uint32_t hash = 0;

	for (uint32_t i = 0;i < len; ++i) {
		hash += k[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

static void configure(struct c_utils_map_conf *conf) {
	if(!conf->cb.hash_fnc)
		conf->cb.hash_fnc = hash_key;

	if(!conf->cb.value_cmp)
		conf->cb.value_cmp = memcmp;

	if(!conf->num_buckets)
		conf->num_buckets = default_buckets;

	if(!conf->growth.ratio <= 0)
		conf->growth.ratio = default_growth_rate;

	if(!conf->growth.trigger <= 0)
		conf->growth.trigger = default_growth_trigger;

	if(conf->shrink.enabled) {
		if(conf->shrink.ratio <= 0)
			conf->shrink.ratio = default_shrink_rate;

		if(conf->shrink.trigger <= 0)
			conf->shrink.trigger = default_shrink_trigger;
	}

}

static void map_destroy(void *map) {
	struct c_utils_map *m = map;

	// If map->buckets is NULL, it signifies we failed during allocation.
	if(!m->buckets)
		return;

	// Clean up resources!!!

	free(m);
}

/// Create a hash map with the requested amount of buckets, the bounds if applicable, and whether to initialize and use rwlocks.
struct c_utils_map *c_utils_map_create() {
	struct c_utils_map_conf conf = {};
	return c_utils_map_create_conf(&conf);
}

struct c_utils_map *c_utils_map_create_conf(struct c_utils_map_conf *conf) {
	if(!conf)
		return NULL;

	configure(conf);

	struct c_utils_map *map;
	if(conf->rc.instance)
		map = c_utils_ref_create(sizeof(*map));
	else
		map = malloc(sizeof(*map));
	if(!map) {
		C_UTILS_LOG_ASSERT(conf->logger, "Failed to create map!");
		goto err;
	}

	map->amount_of_buckets = conf->num_buckets;
	
	C_UTILS_ON_BAD_CALLOC(map->buckets, conf->logger, sizeof(struct c_utils_bucket) * map->amount_of_buckets)
		goto err_buckets;
	
	map->lock = conf->concurrent ? c_utils_scoped_lock_rwlock(NULL, conf->logger) : c_utils_scoped_lock_no_op();
	if(!map->lock) {
		C_UTILS_LOG_ERROR(conf->logger, "Was unable to create the scoped_lock!");
		goto err_lock;
	}

	return map;

	err_lock:
		free(map->buckets);
		map->buckets = NULL;
	err_buckets:
		if(conf->rc.instance)
			C_UTILS_REF_DEC(map);
		else
			free(map);
	err:
		return NULL;
}

/// Add a key-value pair to a hash map.
bool c_utils_map_add(struct c_utils_map *map, void *key, void *value) {
	if(!map)
		return false;

	if(!key) {
		C_UTILS_LOG_ERROR(map->conf.logger, "This map does not support NULL keys!");
		return false;
	} else if (!value) {
		C_UTILS_LOG_ERROR(map->conf.logger, "This map does not support NULL values!");
		return false;
	}

	C_UTILS_SCOPED_WRLOCK(map->lock) {
		// Pick up here! Reminder: Linear Probing!
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
