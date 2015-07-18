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

int get_bucket_index(const char *key, size_t amount_of_buckets){
	return hash_key(key) % amount_of_buckets;
}

DS_Hash_Map_Bucket_t *create_bucket(const char *key, void *value, DS_Hash_Map_Bucket_t *next){
	DS_Hash_Map_Bucket_t *bucket = malloc(sizeof(DS_Hash_Map_Bucket_t));
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
	map->buckets = calloc(amount_of_buckets, sizeof(DS_Hash_Map_Bucket_t *));
	if(!map->buckets){
		MU_LOG_ASSERT(logger, "DS_Hash_Map_create->calloc: \"%s\"\n", strerror(errno));
		goto error;
	}
	return map;

	error:
		if(map){
			free(map);
		}
		return NULL;
}

/// Add a key-value pair to a hash map.
int DS_Hash_Map_add(DS_Hash_Map_t *map, char *key, void *value){

}

/// Obtains the value from the key provided.
void *DS_Hash_Map_get(DS_Hash_Map_t *map, const char *key){
	int index = get_bucket_index(key, map->amount_of_buckets);
	DS_Hash_Map_Bucket_t *bucket = map->buckets[index];
	if(!bucket || !bucket->in_use){
		return NULL;
	}
	while((strcmp(bucket->key, key) != 0) && (bucket = bucket->next)){
		if(!bucket->in_use) continue;
	}
	MU_DEBUG("Search Key: \"%s\" ; Found Key: \"%s\" ; Bucket In Use: \"%s\"\n", key, bucket ? bucket->key : "NULL", bucket ? (bucket->in_use ? "TRUE" : "FALSE") : "FALSE");
	return bucket ? bucket->value : NULL;
}

/// Return and remove the value at the key provided, deleting it if the deletion callback is not NULL.
void *DS_Hash_Map_remove(DS_Hash_Map_t *map, const char *key, DS_delete_cb del){
	int index = get_bucket_index(key, map->amount_of_buckets);
	DS_Hash_Map_Bucket_t *bucket = map->buckets[index];
	MU_DEBUG("Search Key: \"%s\" ; Found Key: \"%s\" ; Bucket In Use: \"%s\"\n", key, bucket ? bucket->key : "NULL", bucket ? (bucket->in_use ? "TRUE" : "FALSE") : "FALSE");


}

/// Determines whether or not the item exists within the map. If the comparator is NULL, it is a pointer-comparison, otherwise it will be based om cmp.
char *DS_Hash_Map_contains_value(DS_Hash_Map_t *map, const void *value, DS_comparator_cb cmp);

/// Uses said callback on all elements inside of the map based on the general callback supplied.
int DS_Hasp_Map_for_each(DS_Hash_Map_t *map, DS_general_cb cb);

/// Will clear the map of all elements, calling the deletion callback on each element if it is not NULL.
int DS_Hash_Map_clear(DS_Hash_Map_t *map, DS_delete_cb del);

/// Determines the size at the time this function is called.
size_t DS_Hash_Map_size(DS_Hash_Map_t *map);

int DS_Hash_Map_destroy(DS_Hash_Map_t *map, DS_delete_cb del);