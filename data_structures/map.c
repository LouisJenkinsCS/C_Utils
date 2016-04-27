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

static const int default_initial = 64;
static const int default_min = 32;
static const int default_max = 1024;
static const double default_growth_rate = 2;
static const double default_growth_trigger = .5;
static const double default_shrink_rate = .5;
static const double default_shrink_trigger = .1;



//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						Map Retrieval Helper Functions                              //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////

static struct c_utils_bucket *get_existing_bucket(const struct c_utils_map *map, const void *key);

static struct c_utils_bucket *get_empty_bucket(const struct c_utils_map *map, const void *key);



//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						Map Key-Value Helper Functions                              //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////

static size_t get_key_size(const struct c_utils_map *map, const void *key);

static uint32_t hash_key(const void *key, size_t len);

static void *value_to_key(struct c_utils_map *map, const void *value);



//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						Map Iterator Functions                                      //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////

static void *head(void *instance, void *pos);

static void *tail(void *instance, void *pos);

static void *next(void *instance, void *pos);

static void *prev(void *instance, void *pos);

static void *curr(void *instance, void *pos);

static void finalize(void *instance, void *pos);



//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						Map Misc. Helper Functions                                  //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////

static bool resize_map(struct c_utils_map *map, size_t size);

static void map_destroy(void *map);



//////////////////////////////////////////////////////////////////////////////////////
//	 																				//
//						Map Configuration Helper Functions                          //
//  																				//
//////////////////////////////////////////////////////////////////////////////////////

static void configure(struct c_utils_map_conf *conf);

static int key_cmp(const struct c_utils_map *map, const void *a, const void *b);

static int value_cmp(const struct c_utils_map *map, const void *a, const void *b);



struct c_utils_map *c_utils_map_create() {
	struct c_utils_map_conf conf = {};
	return c_utils_map_create_conf(&conf);
}

struct c_utils_map *c_utils_map_create_conf(struct c_utils_map_conf *conf) {
	if(!conf)
		return NULL;

	configure(conf);

	struct c_utils_map *map;
	if(conf->flags & C_UTILS_MAP_RC_INSTANCE) {
		struct c_utils_ref_count_conf rc_conf =
		{
			.destructor = map_destroy,
			.logger = conf->logger
		};

		map = c_utils_ref_create_conf(sizeof(*map), &rc_conf);
	} else {
		map = malloc(sizeof(*map));
	}

	if(!map) {
		C_UTILS_LOG_ASSERT(conf->logger, "Failed to create map!");
		goto err;
	}

	map->num_buckets = conf->size.initial;
	map->size = 0;

	C_UTILS_ON_BAD_CALLOC(map->buckets, conf->logger, sizeof(struct c_utils_bucket) * map->num_buckets)
		goto err_buckets;

	map->lock = conf->flags & C_UTILS_MAP_CONCURRENT ? c_utils_scoped_lock_rwlock(NULL, conf->logger) : c_utils_scoped_lock_no_op();
	if(!map->lock) {
		C_UTILS_LOG_ERROR(conf->logger, "Was unable to create the scoped_lock!");
		goto err_lock;
	}

	map->conf = *conf;

	return map;

	err_lock:
		free(map->buckets);
	err_buckets:
		if(conf->flags & C_UTILS_MAP_RC_INSTANCE)
			c_utils_ref_destroy(map);
		else
			free(map);
	err:
		return NULL;
}

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

		// Have we added enough to trigger a growth?
		if((++map->size / (double)map->num_buckets) >= map->conf.growth.trigger)
			resize_map(map, map->num_buckets * map->conf.growth.ratio);

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

		// The caller is obtaining a copy of the value, hence they gain a reference to it.
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

		// If we have a reference to the bucket's key, release it.
		if(map->conf.flags & C_UTILS_MAP_RC_KEY)
			C_UTILS_REF_DEC(bucket->key);

		bucket->in_use = false;
		map->size--;

		// Is shrinking enabled? Do we have enough free space to trigger shrinking?
		if(map->conf.flags & C_UTILS_MAP_SHRINK_ON_TRIGGER)
			if((map->size / (double)map->num_buckets) <= map->conf.shrink.trigger)
				resize_map(map, map->size * map->conf.shrink.ratio);

		// Note that the caller steals our reference to the value.
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
				// If we have a reference to the key, release it.
				if(map->conf.flags & C_UTILS_MAP_RC_KEY)
					C_UTILS_REF_DEC(bucket->key);

				// If we have a reference to the value, release it.
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

		// If we have a reference to the key, release it.
		if(!(map->conf.flags & C_UTILS_MAP_RC_KEY))
			C_UTILS_REF_DEC(bucket->key);

		// If we have a reference to the value, release it. Otherwise, invoke destructor.
		if(!(map->conf.flags & C_UTILS_MAP_RC_VALUE))
			C_UTILS_REF_DEC(bucket->value);
		else
			map->conf.callbacks.destructors.value(bucket->value);

		bucket->in_use = false;
		map->size--;

		// Is shrinking enabled? Do we have enough free space to trigger shrinking?
		if(map->conf.flags & C_UTILS_MAP_SHRINK_ON_TRIGGER)
			if((map->size / (double)map->num_buckets) <= map->conf.shrink.trigger)
				resize_map(map, map->size * map->conf.shrink.ratio);

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
				// If we have a reference to the key, release it. Otherwise, invoke destructor
				if(map->conf.flags & C_UTILS_MAP_RC_KEY)
					C_UTILS_REF_DEC(bucket->key);
				else if(map->conf.callbacks.destructors.key)
					map->conf.callbacks.destructors.key(bucket->key);

				// If we have a reference to the value, release it. Otherwise, invoke destructor.
				if(map->conf.flags & C_UTILS_MAP_RC_VALUE)
					C_UTILS_REF_DEC(bucket->value);
				else
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

		// As the caller is receiving a copy, they now also gain a reference to it.
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

struct c_utils_iterator *c_utils_map_iterator(struct c_utils_map *map) {
	if(!map)
		return NULL;

	struct c_utils_iterator *it;
	struct _c_utils_map_iterator_position *pos;
	C_UTILS_ON_BAD_CALLOC(it, map->conf.logger, sizeof(*it))
		goto err;

	C_UTILS_SCOPED_RDLOCK(map->lock) {
		size_t occupied_buckets = map->size;

		C_UTILS_ON_BAD_CALLOC(pos, map->conf.logger, sizeof(*pos) + sizeof(struct _c_utils_map_data) * occupied_buckets)
			goto err_pos;

		pos->size = occupied_buckets;

		/*
			Copy each key-value pair into the iterator position's data, incrementing the reference
			count of the key-value pair if needed.
		*/
		for(size_t i = 0, j = 0; occupied_buckets &&  i < map->num_buckets; i++) {
			if(!map->buckets[i].in_use)
				continue;

			// We gain a reference to key
			if(map->conf.flags & C_UTILS_MAP_RC_KEY)
				C_UTILS_REF_INC(map->buckets[i].key);

			// We gain a reference to the value
			if(map->conf.flags & C_UTILS_MAP_RC_VALUE)
				C_UTILS_REF_INC(map->buckets[i].value);

			pos->data[j].key = map->buckets[i].key;
			pos->data[j].value = map->buckets[i].value;

			j++;
			occupied_buckets--;
		}
	}

	// Set up callback functions
	it->handle = map;
	it->pos = pos;
	it->head = head;
	it->tail = tail;
	it->next = next;
	it->prev = prev;
	it->curr = curr;
	it->finalize = finalize;

	// Iterator now holds a reference to this map.
	if(map->conf.flags & C_UTILS_MAP_RC_INSTANCE) {
		it->conf.ref_counted = true;
		C_UTILS_REF_INC(map);
	}

	return it;

	err_pos:
		free(it);
	err:
		return NULL;
}

void c_utils_map_destroy(struct c_utils_map *map) {
	if(!map)
		return;

	// If reference counted, just release our reference to it.
	if(map->conf.flags & C_UTILS_MAP_RC_INSTANCE) {
		C_UTILS_REF_DEC(map);
		return;
	}

	// Otherwise call destructor directly.
	map_destroy(map);
}



static struct c_utils_bucket *get_existing_bucket(const struct c_utils_map *map, const void *key) {
	if(!map->size)
		return NULL;

	// Hash key, falling back on default if no hash function given.
	uint32_t hash;
	if(map->conf.callbacks.hash_function)
		hash = map->conf.callbacks.hash_function(key);
	else
		hash = hash_key(key, get_key_size(map, key));

	size_t index = hash % map->num_buckets;

	size_t iterations = 0;
	struct c_utils_bucket *bucket;
	while(iterations++ < map->num_buckets && (bucket = map->buckets + (index++ % map->num_buckets))->in_use && bucket->hash == hash) {
		if(key_cmp(map, key, bucket->key) == 0)
			return bucket;
	}

	return NULL;
}


static struct c_utils_bucket *get_empty_bucket(const struct c_utils_map *map, const void *key) {
	if(map->size == map->num_buckets)
		return NULL;

	// Hash key, falling back on default if no hash function given.
	uint32_t hash;
	if(map->conf.callbacks.hash_function)
		hash = map->conf.callbacks.hash_function(key);
	else
		hash = hash_key(key, get_key_size(map, key));

	size_t index = hash % map->num_buckets;

	size_t iterations = 0;
	struct c_utils_bucket *bucket;
	while(iterations++ < map->num_buckets && (bucket = map->buckets + (index++ % map->num_buckets))->in_use) {
		// If the key is found, then that means it already exists.
		if(key_cmp(map, key, bucket->key) == 0)
			return NULL;
	}

	bucket->hash = hash;
	return bucket;
}



static size_t get_key_size(const struct c_utils_map *map, const void *key) {
	return map->conf.length.key ? map->conf.length.key : strlen(key);
}

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

static void *value_to_key(struct c_utils_map *map, const void *value) {
	C_UTILS_SCOPED_RDLOCK(map->lock) {
		size_t search = map->size;
		for(size_t i = 0; search && i < map->num_buckets; i++) {
			struct c_utils_bucket *bucket = map->buckets + i;
			if(bucket->in_use) {
				if(value_cmp(map, value, bucket->value) == 0)
					return bucket->key;

				search--;
			}
		}

		return NULL;
	}

	C_UTILS_UNACCESSIBLE;
}



static bool resize_map(struct c_utils_map *map, size_t size) {
	C_UTILS_SCOPED_WRLOCK(map->lock){
		if(map->num_buckets == map->conf.size.initial)
			return true;

		if(size < map->conf.size.initial)
			return true;

		size_t used = map->size, index = 0;
		void *keys[used];
		void *values[used];

		// Keep track of key-value pairs
		for(size_t i = 0;i < map->num_buckets;) {
			struct c_utils_bucket *bucket = map->buckets + i;
			if(bucket->in_use) {
				keys[index] = bucket->key;
				values[index++] = bucket->value;
			}

			bucket->in_use = false;
		}

		size_t new_size = size * sizeof(struct c_utils_bucket);
		C_UTILS_ON_BAD_REALLOC(&map->buckets, map->conf.logger, new_size)
			return false;

		memset(&map->buckets, 0, new_size);
		map->num_buckets = size;

		// Re-hash all key-value pairs
		for(size_t i = 0; i < used; i++) {
			struct c_utils_bucket *bucket = get_empty_bucket(map, keys[i]);
			C_UTILS_ASSERT(bucket, map->conf.logger, "Duplicate key found in list!");

			bucket->key = keys[i];
			bucket->value = values[i];
			bucket->in_use = true;
		}

		return true;
	}

	C_UTILS_UNACCESSIBLE;
}

static void configure(struct c_utils_map_conf *conf) {
	if(!conf->size.initial)
		conf->size.initial = default_initial;

	if(!conf->size.min)
		conf->size.min = default_min;

	if(!conf->size.max)
		conf->size.max = default_max;

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

	if(!conf->callbacks.destructors.value)
		conf->callbacks.destructors.value = free;

}


static void map_destroy(void *map) {
	struct c_utils_map *m = map;

	for(size_t i = 0; m->size && i < m->num_buckets; i++) {
		struct c_utils_bucket *bucket = m->buckets + i;

		/*
			If the bucket is in use, then that means that the key and value pairs need to be
			cleaned up as well. In the case that they are reference counted, we release our reference
			to it. If instead there is a destructor, it will invoke that instead.
		*/
		if(bucket->in_use) {
			if(m->conf.flags & C_UTILS_MAP_RC_KEY)
				C_UTILS_REF_DEC(bucket->key);
			else if(m->conf.flags & C_UTILS_MAP_DELETE_ON_DESTROY && m->conf.callbacks.destructors.key)
				m->conf.callbacks.destructors.key(bucket->key);

			if(m->conf.flags & C_UTILS_MAP_RC_VALUE)
				C_UTILS_REF_DEC(bucket->value);
			else if(m->conf.flags & C_UTILS_MAP_DELETE_ON_DESTROY)
				m->conf.callbacks.destructors.value(bucket->value);

			m->size--;
		}
	}

	c_utils_scoped_lock_destroy(m->lock);

	free(m->buckets);
	free(m);
}



/*
	As the map is very configurable, we must check below to determine how we will
	compare keys in this instance of the map. There are three ways to compare a key
	in this map:

	1) Via a direct comparator
	2) By checking each byte up to it's length
	3) Assuming it is a string and using strcmp

	It should be noted that if the creator did not specify a length nor a callback,
	then this invoked undefined behavior. However, by default this allows string keys
	to work.
*/
static int key_cmp(const struct c_utils_map *map, const void *a, const void *b) {
	if(map->conf.callbacks.comparators.key) {
		return map->conf.callbacks.comparators.key(a, b);
	} else {
		if(map->conf.length.key)
			return memcmp(a, b, map->conf.length.key);
		else
			return strcmp(a, b);
	}
}

/*
	As the map is very configurable, we must check below how we wish to
	compare the two values. There are three ways to compare a value:

	1) Via a direct comparator
	2) By checking each byte up to it's length
	3) By pointer address

	Hence this is why we must have extensive checks like this below.
*/
static int value_cmp(const struct c_utils_map *map, const void *a, const void *b) {
	if(map->conf.callbacks.comparators.value)
		return map->conf.callbacks.comparators.value(a, b);
	else if(map->conf.length.value)
		return memcmp(a, b, map->conf.length.value);
	else
		return a != b;
}



static void *head(void *instance, void *pos) {
	struct _c_utils_map_iterator_position *position = pos;

	if(!position->size)
		return NULL;

	return position->data[(position->index = 0)].value;
}

static void *tail(void *instance, void *pos) {
	struct _c_utils_map_iterator_position *position = pos;

	if(!position->size)
		return NULL;

	return position->data[(position->index = position->size - 1)].value;
}

static void *next(void *instance, void *pos) {
	struct _c_utils_map_iterator_position *position = pos;

	// If index is on last position, we cannot go further.
	if(position->index + 1 >= position->size)
		return NULL;

	return position->data[++position->index].value;
}

static void *prev(void *instance, void *pos) {
	struct _c_utils_map_iterator_position *position = pos;

	// If we are on the zero'th position, we cannot go back anymore without overflowing
	if(!position->index)
		return NULL;

	return position->data[--position->index].value;
}

static void *curr(void *instance, void *pos) {
	struct _c_utils_map_iterator_position *position = pos;

	if(!position->size)
		return NULL;

	return position->data[position->index].value;
}

static void finalize(void *instance, void *pos) {
	struct c_utils_map *map = instance;
	struct _c_utils_map_iterator_position *position = pos;
	
	/*
		As the iterator will maintain a reference to the copied data (key-value pairs),
		during finalization we must relinquish that reference as well. Of course, we also
		relinquish the reference over the instance of the map itself too.
	*/
	for(size_t i = 0; i < position->size; i++) {
		if(map->conf.flags & C_UTILS_MAP_RC_KEY)
			C_UTILS_REF_DEC(position->data[i].key);

		if(map->conf.flags & C_UTILS_MAP_RC_VALUE)
			C_UTILS_REF_DEC(position->data[i].value);
	}

	if(map->conf.flags & C_UTILS_MAP_RC_INSTANCE)
		C_UTILS_REF_DEC(map);

	free(pos);
}