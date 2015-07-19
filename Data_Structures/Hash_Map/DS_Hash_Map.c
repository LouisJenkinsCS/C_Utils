#include <DS_Hash_Map.h>

__attribute__((constructor)) static void init_logger(void){
	logger = malloc(sizeof(MU_Logger_t));
	if(!logger){
		MU_DEBUG("Unable to allocate memory for DS_Hash_Map's logger!!!\n");
		return;
	}
	MU_Logger_Init(logger, "DS_Hash_Map.log", "w", MU_ALL);
}

__attribute__((destructor)) static void destroy_logger(void){
	MU_Logger_Destroy(logger);
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

/// Non threaded version of clearing a map.
static int clear_map(DS_Hash_Map_t *map, DS_delete_cb del){
	size_t i = 0, total_buckets = map->amount_of_buckets;
	for(; i < total_buckets; i++){
		for_each_bucket(map->buckets[i], del, 1);
	}
	map->size = 0;
	return 1;
}

static void delete_bucket(DS_Bucket_t *bucket, DS_delete_cb del){
	del(curr_bucket->value);
	free(curr_bucket);
}

static void delete_all_buckets(DS_Bucket_t **buckets, size_t amount_of_buckets, DS_delete_cb del){
	size_t i = 0, total_buckets = map->amount_of_buckets;
	DS_Bucket_t *curr_bucket = NULL, *head_bucket = NULL;
	for(; i < amount_of_buckets; i++){
		if(!(head_bucket = curr_bucket = map->buckets[i])){
			continue;
		}
		while(head_bucket = curr_bucket->next){
			delete_bucket(bucket, del);
			curr_bucket = head_bucket;
		}
		if(curr_bucket){
			delete_bucket(bucket, del);
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
	} while(bucket = bucket->next);
}

static void *get_value_from_bucket(DS_Bucket_t *bucket, const char *key){
	if(!bucket) return NULL;
	do {
		if(!bucket->in_use) continue;
		if(strcmp(bucket->key, key) != 0){
			break;
		}
	} while((bucket = bucket->next);
	MU_DEBUG("Search Key: \"%s\" ; Found Key: \"%s\" ; Bucket In Use: \"%s\"\n", key, bucket ? bucket->key : "NULL", bucket ? (bucket->in_use ? "TRUE" : "FALSE") : "FALSE");
	return bucket ? bucket->value ? NULL;
}

static size_t get_bucket_index(const char *key, size_t amount_of_buckets){
	return hash_key(key) % amount_of_buckets;
}

static int bucket_is_valid(DS_Bucket_t *bucket){
	return bucket && bucket->in_use;
}

static DS_Bucket_t *get_bucket(DS_Bucket_t **buckets, size_t amount_of_buckets, const char *key){
	return buckets[get_bucket_index(key, amount_of_buckets)];
}

static const char *get_key_if_match(DS_Bucket_t *bucket, const void *value, DS_comparator_cb cmp){
	if(!bucket) return NULL;
	do {
		if(!bucket->in_use) continue;
		if(cmp ? cmp(bucket->value, value) == 0 : bucket->value == value){
			break;
		}
	} while(bucket = bucket->next);
	return bucket ? bucket->key : NULL;
}

DS_Bucket_t *create_bucket(const char *key, void *value, DS_Bucket_t *next){
	DS_Bucket_t *bucket = malloc(sizeof(DS_Bucket_t));
	if(!bucket){
		MU_LOG_ASSERT(logger, "create_bucket->malloc: \"%s\"\n", strerror(errno));
		return NULL;
	}
	bucket->key = key;
	bucket->value = value;
	bucket->next = next;
	return bucket;
}

/// Create a hash map with the requested amount of buckets, the bounds if applicable, and whether to initialize and use rwlocks.
DS_Hash_Map_t *DS_Hash_Map_create(size_t amount_of_buckets, unsigned char init_locks){
	DS_Hash_Map_t *map = malloc(sizeof(DS_Hash_Map_t));
	if(!map){
		MU_LOG_ASSERT(logger, "DS_Hash_Map_create->malloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	map->buckets = calloc(amount_of_buckets, sizeof(DS_Bucket_t *));
	if(!map->buckets){
		MU_LOG_ASSERT(logger, "DS_Hash_Map_create->calloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	map->size = 0;
	return map;

	error:
		if(map){
			free(map);
		}
		return NULL;
}

/// Add a key-value pair to a hash map.
int DS_Hash_Map_add(DS_Hash_Map_t *map, char *key, void *value){
	size_t index = get_bucket_index(key, map->amount_of_buckets);
	DS_Bucket_t *bucket = map->buckets[index];
	if(!bucket){
		bucket = map->buckets[index]  = calloc(1, sizeof(DS_Bucket_t));
		if(!bucket){
			MU_LOG_ASSERT(logger, "DS_Hash_Map_add->calloc: \"%s\"\n", strerror(errno));
			return 0;
		}
		bucket->value = value;
		bucket->key = key;
		bucket->in_use = 1;
		map->size++;
		return 1;
	}
	void *key_exists = get_value_from_bucket(bucket, key);
	if(key_exists){
		return 0;
	}
	do {
		if(!bucket->in_use){
			bucket->key = key;
			bucket->value = value;
			bucket->in_use = 1;
			map->size++;
			break;
		} else continue;
	} while(bucket = bucket->next);
	return 1;
}

/// Obtains the value from the key provided.
void *DS_Hash_Map_get(DS_Hash_Map_t *map, const char *key){
	DS_Bucket_t *bucket = get_bucket(map->buckets, map->amount_of_buckets, key);
	if(!bucket_is_valid(bucket)){
		return NULL;
	}
	void *value = get_value_from_bucket(bucket, key);
	return value;
}

/// Return and remove the value at the key provided, deleting it if the deletion callback is not NULL.
void *DS_Hash_Map_remove(DS_Hash_Map_t *map, const char *key, DS_delete_cb del){
	DS_Bucket_t *bucket = get_bucket(map->buckets, map->amount_of_buckets, key);
	if(!bucket_is_valid(bucket)){
		return NULL;
	}
	void *value = get_value_from_bucket(bucket, key);
	bucket->in_use = 0;
	map->size--;
	return value;
}

/// Determines whether or not the item exists within the map. If the comparator is NULL, it is a pointer-comparison, otherwise it will be based om cmp.
const char *DS_Hash_Map_contains(DS_Hash_Map_t *map, const void *value, DS_comparator_cb cmp){
	size_t i = 0, total_buckets = map->amount_of_buckets;
	char *key = NULL;
	DS_Bucket_t *bucket = NULL;
	// O(N) complexity.
	for(; i < total_buckets; i++){
		bucket = map->buckets[i];
		if(key = get_key_if_match(bucket, value, cmp)){
			break;
		}
	}
	return key;
}

/// Uses said callback on all elements inside of the map based on the general callback supplied.
int DS_Hasp_Map_for_each(DS_Hash_Map_t *map, DS_general_cb cb){
	size_t i = 0, total_buckets = map->amount_of_buckets;
	for(; i < total_buckets; i++){
		for_each_bucket(map->buckets[i], cb, 0);
	}
	return 1;
}

/// Will clear the map of all elements, calling the deletion callback on each element if it is not NULL.
int DS_Hash_Map_clear(DS_Hash_Map_t *map, DS_delete_cb del){
	return clear_map(map, del);
}

/// Determines the size at the time this function is called.
size_t DS_Hash_Map_size(DS_Hash_Map_t *map){
	return map->size;
}

int DS_Hash_Map_destroy(DS_Hash_Map_t *map, DS_delete_cb del){
	delete_all_buckets(map->buckets, map->amount_of_buckets, del);
	return 1;
}