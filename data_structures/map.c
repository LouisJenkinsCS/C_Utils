#include "map.h"

#include "../misc/alloc_check.h"
#include "../misc/argument_check.h"
#include "../io/logger.h"
#include "../threading/scoped_lock.h"
#include "../memory/ref_count.h"

struct c_utils_bucket {
	/// Hash.
	uint32_t hash;
	/// Key
	void *key;
	/// Value
	void *value;
	/// Determines whether or not the bucket is in use, to allow for "caching".
	volatile bool in_use;
};

struct c_utils_map {
	/// Array of all bucket head nodes.
	struct c_utils_bucket *buckets;
	/// The size of the hash map.
	size_t size;
	/// Maximum amount of buckets.
	size_t num_buckets;
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

static bool expand_map(struct c_utils_map *map);

static bool shrink_map(struct c_utils_map *map);

static void *value_to_key(struct c_utils_map *map, const void *key);

/// Very simple and straight forward hash function. Bob Jenkin's hash. Default hash if none supplied.
static uint32_t hash_key(const void *key, size_t len) {
	const unsigned char *k = key;
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
	if(!conf->callbacks.hash_function)
		conf->callbacks.hash_function = hash_key;

	if(!conf->num_buckets)
		conf->num_buckets = default_buckets;

	if(!conf->growth.ratio <= 0)
		conf->growth.ratio = default_growth_rate;

	if(!conf->growth.trigger <= 0)
		conf->growth.trigger = default_growth_trigger;

	if(conf->flags & C_UTILS_MAP_SHRINK_ON_TRIGGER) {
		if(conf->shrink.ratio <= 0)
			conf->shrink.ratio = default_shrink_rate;

		if(conf->shrink.trigger <= 0)
			conf->shrink.trigger = default_shrink_trigger;
	}

}

/*
	If the comparator for the key is specified, we use it to determine if the keys are
	in deed the same. This is useful because we use linear probing with this map, and hence
	need to determine if this is the exact one needed.

	If a comparator has not been specified, we instead need to provide a more generic
	comparator, by using memcmp to check each byte to determine if they are equal.
*/
static bool keys_equal(const struct c_utils_map *map, const void *first, size_t first_len, const void *second, size_t second_len) {
	if(map->conf.callbacks.comparators.key)
		return map->conf.callbacks.comparators.key(first, second) == 0;
	else
		return memcmp(first, second, first_len > second_len ? second_len : first_len);
}

static size_t get_key_size(const struct c_utils_map *map, const void *key) {
	return map->conf.key_len ? map->conf.key_len : strlen(key);
}

static struct c_utils_bucket *get_existing_bucket(const struct c_utils_map *map, const void *key) {
	size_t key_len = get_key_size(map, key);
	uint32_t hash = map->conf.callbacks.hash_function(key, key_len);
	size_t index = hash % map->num_buckets;

	size_t iterations = 0;
	struct c_utils_bucket *bucket;
	while(iterations++ < map->num_buckets && (bucket = map->buckets + (index++ % map->num_buckets))->in_use && bucket->hash == hash) {
		size_t bucket_key_len = get_key_size(map, bucket->key);

		if(keys_equal(map, key, key_len, bucket->key, bucket_key_len))
			return bucket;
	}

	return NULL;
}


static struct c_utils_bucket *get_empty_bucket(const struct c_utils_map *map, const void *key) {
	if(map->size == map->num_buckets)
		return false;

	size_t key_len = get_key_size(map, key);
	uint32_t hash = map->conf.callbacks.hash_function(key, key_len);
	size_t index = hash % map->num_buckets;

	size_t iterations = 0;
	struct c_utils_bucket *bucket;
	while(iterations++ < map->num_buckets && (bucket = map->buckets + (index++ % map->num_buckets))->in_use) {
		size_t bucket_key_len = get_key_size(map, bucket->key);

		if(keys_equal(map, key, key_len, bucket->key, bucket_key_len))
			return NULL;
	}

	bucket->hash = hash;
	return bucket;
}

static void map_destroy(void *map) {
	struct c_utils_map *m = map;

	// If map->buckets is NULL, it signifies we failed during allocation.
	if(!m->buckets)
		return;

	for(size_t i = 0; m->size && i < m->num_buckets; i++) {
		struct c_utils_bucket bucket = m->buckets[i];
		if(bucket.in_use) {
			if(m->conf.flags & C_UTILS_MAP_RC_KEY)
				C_UTILS_REF_DEC(bucket.key);
			else if(m->conf.flags & C_UTILS_MAP_DELETE_ON_DESTROY && m->conf.callbacks.destructors.key)
				m->conf.callbacks.destructors.key(bucket.key);

			if(m->conf.flags & C_UTILS_MAP_RC_VALUE)
				C_UTILS_REF_DEC(bucket.value);
			else if(m->conf.flags & C_UTILS_MAP_DELETE_ON_DESTROY && m->conf.callbacks.destructors.value)
				m->conf.callbacks.destructors.value(bucket.value);

			m->size--;
		}
	}

	free(m->buckets);

	c_utils_scoped_lock_destroy(m->lock);

	free(m);
}

struct c_utils_map *c_utils_map_create() {
	struct c_utils_map_conf conf = {};
	return c_utils_map_create_conf(&conf);
}

struct c_utils_map *c_utils_map_create_conf(struct c_utils_map_conf *conf) {
	if(!conf)
		return NULL;

	configure(conf);

	struct c_utils_map *map;
	// Zero the ref_count data plz.
	if(conf->flags & C_UTILS_MAP_RC_INSTANCE)
		map = c_utils_ref_create(sizeof(*map));
	else
		map = malloc(sizeof(*map));
	if(!map) {
		C_UTILS_LOG_ASSERT(conf->logger, "Failed to create map!");
		goto err;
	}

	map->num_buckets = conf->num_buckets;
	
	C_UTILS_ON_BAD_CALLOC(map->buckets, conf->logger, sizeof(struct c_utils_bucket) * map->num_buckets)
		goto err_buckets;
	
	map->lock = conf->flags & C_UTILS_MAP_CONCURRENT ? c_utils_scoped_lock_rwlock(NULL, conf->logger) : c_utils_scoped_lock_no_op();
	if(!map->lock) {
		C_UTILS_LOG_ERROR(conf->logger, "Was unable to create the scoped_lock!");
		goto err_lock;
	}

	return map;

	err_lock:
		free(map->buckets);
		map->buckets = NULL;
	err_buckets:
		if(conf->flags & C_UTILS_MAP_RC_INSTANCE)
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
		struct c_utils_bucket *bucket = get_empty_bucket(map, key);
		if(!bucket)
			return false;

		bucket->key = key;
		bucket->value = value;
		bucket->in_use = true;

		if((double)(++map->size / map->num_buckets) >= map->conf.growth.trigger)
			expand_map(map);

		return true;
	}

	C_UTILS_UNACCESSIBLE;
}

void *c_utils_map_get(struct c_utils_map *map, const void *key) {
	if(!map || !map->size)
		return false;

	if(!key) {
		C_UTILS_LOG_ERROR(map->conf.logger, "This map does not support NULL keys!");
		return false;
	}
	
	C_UTILS_SCOPED_RDLOCK(map->lock) {
		struct c_utils_bucket *bucket = get_existing_bucket(map, key);
		if(!bucket)
			return NULL;

		void *value = bucket->value;

		/*
			The caller is acquiring a new reference to the value. This is because
			the map still contains it's own reference, and this ensures that the value
			remains valid, even after we release the reader lock.
		*/
		if(map->conf.flags & C_UTILS_MAP_RC_VALUE)
			C_UTILS_REF_INC(value);

		return value;
	}

	C_UTILS_UNACCESSIBLE;
}

void *c_utils_map_remove(struct c_utils_map *map, const void *key) {
	if(!map || !map->size)
		return false;

	if(!key) {
		C_UTILS_LOG_ERROR(map->conf.logger, "This map does not support NULL keys!");
		return false;
	}

	C_UTILS_SCOPED_WRLOCK(map->lock) {
		struct c_utils_bucket *bucket = get_existing_bucket(map, key);
		if(!bucket)
			return NULL;

		if(!map->conf.flags & C_UTILS_MAP_RC_KEY)
			C_UTILS_REF_DEC(bucket->key);

		bucket->in_use = false;
		map->size--;

		return bucket->value;
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_map_remove_all(struct c_utils_map *map) {
	if(!map || !map->size)
		return;

	C_UTILS_SCOPED_WRLOCK(map->lock) {
		for(size_t i = 0; i < map->num_buckets; i++) {
			struct c_utils_bucket *bucket = map->buckets + i;
			if(bucket->in_use) {
				if(map->conf.flags & C_UTILS_MAP_RC_KEY)
					C_UTILS_REF_DEC(bucket->key);

				if(map->conf.flags & C_UTILS_MAP_RC_VALUE)
					C_UTILS_REF_DEC(bucket->value);

				bucket->in_use = false;
				map->size--;
			}
		}
	}
}

bool c_utils_map_delete(struct c_utils_map *map, const void *key) {
	if(!map || !map->size)
		return false;

	if(!key) {
		C_UTILS_LOG_ERROR(map->conf.logger, "This map does not support NULL keys!");
		return false;
	}

	C_UTILS_SCOPED_WRLOCK(map->lock) {
		struct c_utils_bucket *bucket = get_existing_bucket(map, key);
		if(!bucket)
			return false;

		if(!map->conf.flags & C_UTILS_MAP_RC_KEY)
			C_UTILS_REF_DEC(bucket->key);

		if(!map->conf.flags & C_UTILS_MAP_RC_VALUE)
			C_UTILS_REF_DEC(bucket->value);
		else if (map->conf.callbacks.destructors.value)
			map->conf.callbacks.destructors.value(bucket->value);

		bucket->in_use = false;
		map->size--;

		return true;
	}

	C_UTILS_UNACCESSIBLE;
}

void c_utils_map_delete_all(struct c_utils_map *map) {
	if(!map || !map->size)
		return;

	C_UTILS_SCOPED_WRLOCK(map->lock) {
		for(size_t i = 0; i < map->num_buckets; i++) {
			struct c_utils_bucket *bucket = map->buckets + i;
			if(bucket->in_use) {
				if(map->conf.flags & C_UTILS_MAP_RC_KEY)
					C_UTILS_REF_DEC(bucket->key);
				else if(map->conf.callbacks.destructors.key)
					map->conf.callbacks.destructors.key(bucket->key);

				if(map->conf.flags & C_UTILS_MAP_RC_VALUE)
					C_UTILS_REF_DEC(bucket->value);
				else if(map->conf.callbacks.destructors.value)
					map->conf.callbacks.destructors.value(bucket->value);

				bucket->in_use = false;
				map->size--;
			}
		}
	}
}

const void *c_utils_map_contains(struct c_utils_map *map, const void *value) {
	if(!map || !map->size)
		return false;

	if(!value) {
		C_UTILS_LOG_ERROR(map->conf.logger, "This map does not support NULL values!");
		return false;
	}

	C_UTILS_SCOPED_RDLOCK(map->lock) {
		void *key = value_to_key(map, value);
		if(!key)
			return NULL;

		if(map->conf.flags & C_UTILS_MAP_RC_KEY)
			C_UTILS_REF_INC(key);

		return key;
	}

	C_UTILS_UNACCESSIBLE;
}

/// Uses said callback on all elements inside of the map based on the general callback supplied.
bool c_utils_map_for_each(struct c_utils_map *map, void (*callback)(const void *key, const void *value)) {
	if(!map || !map->size)
		return false;

	if(!callback) {
		C_UTILS_LOG_ERROR(map->conf.logger, "Callback cannot be NULL!");
		return false;
	}

	C_UTILS_SCOPED_RDLOCK(map->lock) {
		for(size_t i = 0; i < map->num_buckets; i++) {
			struct c_utils_bucket *bucket = map->buckets + i;
			if(bucket->in_use)
				callback(bucket->key, bucket->value);
		}

		return true;
	}
	
	C_UTILS_UNACCESSIBLE;
}

/// Determines the size at the time this function is called.
size_t c_utils_map_size(struct c_utils_map *map) {
	if(!map)
		return 0;

	C_UTILS_SCOPED_RDLOCK(map->lock)
		return map->size;

	C_UTILS_UNACCESSIBLE;
}

void c_utils_map_destroy(struct c_utils_map *map) {
	if(!map)
		return;

	if(map->conf.flags & C_UTILS_MAP_RC_INSTANCE)
		C_UTILS_REF_DEC(map);

	map_destroy(map);
}
