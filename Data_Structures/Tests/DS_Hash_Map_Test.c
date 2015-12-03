#include <DS_Hash_Map.h>
#include <string.h>
#include <MU_Logger.h>

static MU_Logger_t *logger = NULL;
const int buckets = 31;
const int synchronized = 0;

int main(void){
	logger = MU_Logger_create("./Data_Structures/Logs/DS_Hash_Map_Test.log", "w", MU_ALL);
	assert(logger);
	char *keys[] = {
		"Host",
		"User-Agent",
		"Cache-Control",
		"Accept",
		"Accept-Language",
		"Keep-Alive",
		"Connection",
		"Cookie"
	};
	char *values[] = {
		"net.tutsplus.com",
		"Mozilla/5.0 (Windows; U; Windows NT 6.1; en-US; rv:1.9.1.5) Gecko",
		"no-cache",
		"text/html,application/xhtml+xml,application/xml;q=0.9,*/",
		"en-us,en;q=0.5",
		"300",
		"keep-alive",
		"PHPSESSID=r2t5uvjq435r4q7ib3vtdjq120"
	};
	int i = 0;
	MU_DEBUG("DS_HASH_MAP_KEY_SIZE: %d", DS_HASH_MAP_KEY_SIZE);
	MU_LOG_VERBOSE(logger, "Logging all Key-Value pairs!");
	char *all_pairs, *old_str;
	asprintf(&all_pairs, "{ \n");
	for(; i < 6; i++){
		old_str = all_pairs;
		asprintf(&all_pairs, "%s%s: %s,\n", all_pairs, keys[i], values[i]);
		free(old_str);
	}
	old_str = all_pairs;
	asprintf(&all_pairs, "%s}", all_pairs);
	free(old_str);
	MU_LOG_VERBOSE(logger, "%s", all_pairs);
	free(all_pairs);
	MU_LOG_INFO(logger, "Creating Hash Map...");
	DS_Hash_Map_t *map = DS_Hash_Map_create(buckets, synchronized);
	MU_ASSERT(map, logger, "DS_Hash_Map_create: \"Was unable to allocate hash map!\"");
	MU_LOG_INFO(logger, "Adding all key-value pairs to hash map...");
	for(i = 0; i < 6; i++){
		int was_added = DS_Hash_Map_add(map, keys[i], values[i]);
		MU_ASSERT(was_added, logger, "DS_Hash_Map_add: \"Was unable to add key: \"%s\" with value: \"%s\"!\"", keys[i], values[i]);
	}
	MU_LOG_INFO(logger, "Printing all Key-Value pairs from hash map!");
	size_t size;
	char **arr = DS_Hash_Map_key_value_to_string(map, NULL, ": ", NULL, &size, NULL);
	for(i = 0; i < size; i++){
		MU_DEBUG("%s", arr[i]);
		free(arr[i]);
	}
	free(arr);
	MU_LOG_INFO(logger, "Retrieivng all values from keys from hash map...");
	for(i--; i > 0; i--){
		char *value_retrieved = DS_Hash_Map_get(map, keys[i]);
		MU_ASSERT(value_retrieved && strcmp(value_retrieved, values[i]) == 0, logger,
			"DS_Hash_Map_get: \"Was unable to retrieve the right value from key: \"%s\";Expected: \"%s\", but received \"%s\"!\"",
			keys[i], values[i], value_retrieved ? value_retrieved : "NULL");
	}
	MU_LOG_INFO(logger, "Removing Key-Value pair (\"%s\" : \"%s\")...", keys[3], values[3]);
	MU_ASSERT(DS_Hash_Map_remove(map, keys[3], NULL), logger, "DS_Hash_Map_remove: \"Was unable to remove key: \"%s\"!\"", keys[3]);
	MU_LOG_INFO(logger, "Testing for removal of Key-Value pair...");
	void *was_found = DS_Hash_Map_get(map, keys[3]);
	MU_ASSERT(!was_found, logger, "DS_Hash_Map_get: \"Was unable to correctly remove key: \"%s\"!", keys[3]);
	MU_LOG_INFO(logger, "Testing for adding duplicate key...");
	MU_ASSERT(!DS_Hash_Map_add(map, keys[2], values[2]), logger, "DS_Hash_Map_add: \"Was unable to prevent duplicate key: \"%s\" and value: \"%s\"!\"",
		keys[2], values[2]);
	MU_LOG_INFO(logger, "Testing retrieval of key by value...");
	MU_ASSERT(strcmp(keys[1], DS_Hash_Map_contains(map, values[1], (void *)strcmp)) == 0, logger, "DS_Hash_Map_contains: \"Was unable to find value inside of map!\"");
	MU_LOG_INFO(logger, "Destroy Hash Map...");
	MU_ASSERT(DS_Hash_Map_destroy(map, NULL), logger, "DS_Hash_Map_destroy: \"Was unable to destroy map!\"");
	MU_LOG_INFO(logger, "Success!");
	MU_Logger_destroy(logger);
	return 0;
}