#include "DS_Hash_Map.h"

static MU_Logger_t *logger = NULL;

__attribute__((constructor)) static void init_logger(void){
	logger = MU_Logger_create("DS_Hash_Map.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_destroy(logger);
	free(logger);
}

/// Very simple and straight forward hash function. Bob Jenkin's hash.
static uint32_t hash_key(const char *key){
	uint32_t hash = 0, i = 0;
	for(;i < strlen(key); ++i){
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}

static void delete_bucket(DS_Bucket_t *bucket, DS_delete_cb del){
	if(del){
		del(bucket->value);
	}
	free(bucket);
}

static void delete_all_buckets(DS_Bucket_t **buckets, size_t amount_of_buckets, DS_delete_cb del){
	size_t i = 0;
	DS_Bucket_t *curr_bucket = NULL, *head_bucket = NULL;
	for(; i < amount_of_buckets; i++){
		if(!(head_bucket = curr_bucket = buckets[i])){
			continue;
		}
		while((head_bucket = curr_bucket->next)){
			delete_bucket(curr_bucket, del);
			curr_bucket = head_bucket;
		}
		if(curr_bucket){
			delete_bucket(curr_bucket, del);
		}
	}
}

static void for_each_bucket(DS_Bucket_t *bucket, DS_general_cb cb, unsigned char done_with_bucket){
	if(!bucket) return;
	do {
		if(!bucket->in_use) continue;
		cb(bucket->value);
		if(done_with_bucket){
			bucket->in_use = 0;
		}
	} while((bucket = bucket->next));
}

/// Non threaded version of clearing a map.
static int clear_map(DS_Hash_Map_t *map, DS_delete_cb del){
	size_t i = 0, total_buckets = map->amount_of_buckets;
	for(; i < total_buckets; i++){
		for_each_bucket(map->buckets[i], del, 1);
	}
	map->size = 0;
	return 1;
}

static void *get_value_from_bucket(DS_Bucket_t *bucket, const char *key){
	if(!bucket) return NULL;
	do {
		if(!bucket->in_use) continue;
		if(strcmp(bucket->key, key) == 0){
			break;
		}
	} while((bucket = bucket->next));
	MU_LOG_VERBOSE(logger, "Search Key: '%s'; Found Key: %s", key, MU_ARG_BOOL_EVAL(strcmp(key, bucket->key) == 0));
	return bucket ? bucket->value : NULL;
}

static size_t get_bucket_index(const char *key, size_t amount_of_buckets){
	return hash_key(key) % amount_of_buckets;;
}

static int bucket_is_valid(DS_Bucket_t *bucket){
	return bucket && bucket->in_use;
}

static DS_Bucket_t *get_bucket(DS_Bucket_t **buckets, size_t amount_of_buckets, const char *key){
	return buckets[get_bucket_index(key, amount_of_buckets)];
}

static char *get_key_if_match(DS_Bucket_t *bucket, const void *value, DS_comparator_cb cmp){
	if(!bucket) return NULL;
	do {
		if(!bucket->in_use) continue;
		if(cmp ? cmp(bucket->value, value) == 0 : bucket->value == value){
			break;
		}
	} while((bucket = bucket->next));
	return bucket ? bucket->key : NULL;
}

static DS_Bucket_t *create_bucket(char *key, void *value, DS_Bucket_t *next){
	DS_Bucket_t *bucket = malloc(sizeof(DS_Bucket_t));
	if(!bucket){
		MU_LOG_ASSERT(logger, "create_bucket->malloc: \"%s\"", strerror(errno));
		return NULL;
	}
	bucket->key = key;
	bucket->value = value;
	bucket->next = next;
	return bucket;
}

/// Create a hash map with the requested amount of buckets, the bounds if applicable, and whether to initialize and use rwlocks.
DS_Hash_Map_t *DS_Hash_Map_create(size_t amount_of_buckets, unsigned char init_locks){
	DS_Hash_Map_t *map = calloc(1, sizeof(DS_Hash_Map_t));
	if(!map){
		MU_LOG_ASSERT(logger, "DS_Hash_Map_create->malloc: \"%s\"", strerror(errno));
		goto error;
	}
	map->amount_of_buckets = amount_of_buckets ? amount_of_buckets : 1;
	map->buckets = calloc(map->amount_of_buckets, sizeof(DS_Bucket_t *));
	if(!map->buckets){
		MU_LOG_ASSERT(logger, "DS_Hash_Map_create->calloc: \"%s\"", strerror(errno));
		goto error;
	}
	if(init_locks){
		map->lock = malloc(sizeof(pthread_rwlock_t));
		if(!map->lock){
			MU_LOG_ASSERT(logger, "DS_Hash_Map_create->malloc: \"%s\"", strerror(errno));
			goto error;
		}
	}
	int is_initialized = 1;
	MU_COND_RWLOCK_INIT(map->lock, NULL, is_initialized, logger);
	if(!is_initialized){
		goto error;
	}
	return map;

	error:
		if(map){
			MU_COND_RWLOCK_DESTROY(map->lock, logger);
			free(map);
		}
		return NULL;
}

/// Add a key-value pair to a hash map.
bool DS_Hash_Map_add(DS_Hash_Map_t *map, char *key, void *value){
	MU_ARG_CHECK(logger, false, map, key);
	MU_COND_RWLOCK_WRLOCK(map->lock, logger);
	size_t index = get_bucket_index(key, map->amount_of_buckets);
	DS_Bucket_t *bucket = map->buckets[index];
	if(!bucket){
		bucket = (map->buckets[index] = create_bucket(key, value, NULL));
		if(!bucket){
			MU_LOG_ERROR(logger, "DS_Hash_Map_add->create_bucket: \"Was unable to create a bucket!\"");
			MU_COND_RWLOCK_UNLOCK(map->lock, logger);
			return false;
		}
		bucket->in_use = 1;
		map->size++;
		MU_COND_RWLOCK_UNLOCK(map->lock, logger);
		return true;
	}
	void *key_exists = get_value_from_bucket(bucket, key);
	if(key_exists){
		MU_COND_RWLOCK_UNLOCK(map->lock, logger);
		return false;
	}
	do {
		if(!bucket->in_use){
			bucket->key = key;
			bucket->value = value;
			bucket->in_use = 1;
			map->size++;
			break;
		} else if(!bucket->next){
			bucket->next = create_bucket(key, value, NULL);
			bucket->next->in_use = 1;
			break;
		}
	} while((bucket = bucket->next));
	MU_COND_RWLOCK_UNLOCK(map->lock, logger);
	return true;
}

/// Obtains the value from the key provided.
void *DS_Hash_Map_get(DS_Hash_Map_t *map, const char *key){
	MU_ARG_CHECK(logger, NULL, map, map && map->size, map && map->buckets, key);
	MU_COND_RWLOCK_RDLOCK(map->lock, logger);
	DS_Bucket_t *bucket = get_bucket(map->buckets, map->amount_of_buckets, key);
	if(!bucket_is_valid(bucket)){
		MU_COND_RWLOCK_UNLOCK(map->lock, logger);
		return NULL;
	}
	void *value = get_value_from_bucket(bucket, key);
	MU_COND_RWLOCK_UNLOCK(map->lock, logger);
	return value;
}

/// Return and remove the value at the key provided, deleting it if the deletion callback is not NULL.
void *DS_Hash_Map_remove(DS_Hash_Map_t *map, const char *key, DS_delete_cb del){
	MU_ARG_CHECK(logger, NULL, map, map && map->buckets, map && map->size, key);
	MU_COND_RWLOCK_WRLOCK(map->lock, logger);
	DS_Bucket_t *bucket = get_bucket(map->buckets, map->amount_of_buckets, key);
	if(!bucket_is_valid(bucket)){
		MU_COND_RWLOCK_UNLOCK(map->lock, logger);
		return NULL;
	}
	void *value = get_value_from_bucket(bucket, key);
	bucket->in_use = 0;
	map->size--;
	MU_COND_RWLOCK_UNLOCK(map->lock, logger);
	return value;
}

/// Determines whether or not the item exists within the map. If the comparator is NULL, it is a pointer-comparison, otherwise it will be based om cmp.
const char *DS_Hash_Map_contains(DS_Hash_Map_t *map, const void *value, DS_comparator_cb cmp){
	MU_ARG_CHECK(logger, NULL, map, map && map->buckets, map && map->size);
	MU_COND_RWLOCK_RDLOCK(map->lock, logger);
	size_t i = 0, total_buckets = map->amount_of_buckets;
	char *key = NULL;
	// O(N) complexity.
	for(; i < total_buckets; i++){
		DS_Bucket_t *bucket = map->buckets[i];
		if((key = get_key_if_match(bucket, value, cmp))){
			break;
		}
	}
	MU_COND_RWLOCK_UNLOCK(map->lock, logger);
	return key;
}

/// Uses said callback on all elements inside of the map based on the general callback supplied.
bool DS_Hasp_Map_for_each(DS_Hash_Map_t *map, DS_general_cb callback_function){
	MU_ARG_CHECK(logger, false, map, map && map->buckets, map && map->size, callback_function);
	MU_COND_RWLOCK_RDLOCK(map->lock, logger);
	size_t i = 0, total_buckets = map->amount_of_buckets;
	for(; i < total_buckets; i++){
		for_each_bucket(map->buckets[i], callback_function, 0);
	}
	MU_COND_RWLOCK_UNLOCK(map->lock, logger);
	return 1;
}

/// Will clear the map of all elements, calling the deletion callback on each element if it is not NULL.
bool DS_Hash_Map_clear(DS_Hash_Map_t *map, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, map, map && map->buckets, map && map->size);
	MU_COND_RWLOCK_WRLOCK(map->lock, logger);
	bool successful = clear_map(map, del);
	MU_COND_RWLOCK_UNLOCK(map->lock, logger);
	return successful;
}

/// Determines the size at the time this function is called.
size_t DS_Hash_Map_size(DS_Hash_Map_t *map){
	MU_ARG_CHECK(logger, 0, map, map && map->buckets, map && map->size);
	MU_COND_RWLOCK_RDLOCK(map->lock, logger);
	size_t size = map->size;
	MU_COND_RWLOCK_UNLOCK(map->lock, logger);
	return size;
}

bool DS_Hash_Map_destroy(DS_Hash_Map_t *map, DS_delete_cb del){
	MU_ARG_CHECK(logger, false, map);
	MU_COND_RWLOCK_WRLOCK(map->lock, logger);
	delete_all_buckets(map->buckets, map->amount_of_buckets, del);
	map->amount_of_buckets = 0;
	map->size = 0;
	map->buckets = NULL;
	MU_COND_RWLOCK_UNLOCK(map->lock, logger);
	MU_COND_RWLOCK_DESTROY(map->lock, logger);
	free(map);
	return true;
}