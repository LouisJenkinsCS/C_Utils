#include <DS_Hash_Map.h>
#include <string.h>
#include <MU_Logger.h>

static struct c_utils_logger *logger = NULL;
const int buckets = 31;
const int synchronized = 0;

int main(void) {
	logger = MU_Logger_create("./Data_Structures/Logs/c_utils_map_Test.log", "w", MU_ALL);
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
	C_UTILS_LOG_VERBOSE(logger, "Logging all Key-Value pairs!");
	char *all_pairs, *old_str;
	asprintf(&all_pairs, "{ \n");
	for (; i < 6; i++) {
		old_str = all_pairs;
		asprintf(&all_pairs, "%s%s: %s,\n", all_pairs, keys[i], values[i]);
		free(old_str);
	}
	old_str = all_pairs;
	asprintf(&all_pairs, "%s}", all_pairs);
	free(old_str);
	C_UTILS_LOG_VERBOSE(logger, "%s", all_pairs);
	free(all_pairs);
	C_UTILS_LOG_INFO(logger, "Creating Hash Map...");
	struct c_utils_map *map = c_utils_map_create(buckets, synchronized);
	MU_ASSERT(map, logger, "c_utils_map_create: \"Was unable to allocate hash map!\"");
	C_UTILS_LOG_INFO(logger, "Adding all key-value pairs to hash map...");
	for (i = 0; i < 6; i++) {
		int was_added = c_utils_map_add(map, keys[i], values[i]);
		MU_ASSERT(was_added, logger, "c_utils_map_add: \"Was unable to add key: \"%s\" with value: \"%s\"!\"", keys[i], values[i]);
	}
	C_UTILS_LOG_INFO(logger, "Printing all Key-Value pairs from hash map!");
	size_t size;
	char **arr = c_utils_map_key_value_to_string(map, NULL, ": ", NULL, &size, NULL);
	for (i = 0; i < size; i++) {
		MU_DEBUG("%s", arr[i]);
		free(arr[i]);
	}
	free(arr);
	C_UTILS_LOG_INFO(logger, "Retrieivng all values from keys from hash map...");
	for (i--; i > 0; i--) {
		char *value_retrieved = c_utils_map_get(map, keys[i]);
		MU_ASSERT(value_retrieved && strcmp(value_retrieved, values[i]) == 0, logger,
			"c_utils_map_get: \"Was unable to retrieve the right value from key: \"%s\";Expected: \"%s\", but received \"%s\"!\"",
			keys[i], values[i], value_retrieved ? value_retrieved : "NULL");
	}
	C_UTILS_LOG_INFO(logger, "Removing Key-Value pair (\"%s\" : \"%s\")...", keys[3], values[3]);
	MU_ASSERT(c_utils_map_remove(map, keys[3], NULL), logger, "c_utils_map_remove: \"Was unable to remove key: \"%s\"!\"", keys[3]);
	C_UTILS_LOG_INFO(logger, "Testing for removal of Key-Value pair...");
	void *was_found = c_utils_map_get(map, keys[3]);
	MU_ASSERT(!was_found, logger, "c_utils_map_get: \"Was unable to correctly remove key: \"%s\"!", keys[3]);
	C_UTILS_LOG_INFO(logger, "Testing for adding duplicate key...");
	MU_ASSERT(!c_utils_map_add(map, keys[2], values[2]), logger, "c_utils_map_add: \"Was unable to prevent duplicate key: \"%s\" and value: \"%s\"!\"",
		keys[2], values[2]);
	C_UTILS_LOG_INFO(logger, "Testing retrieval of key by value...");
	MU_ASSERT(strcmp(keys[1], c_utils_map_contains(map, values[1], (void *)strcmp)) == 0, logger, "c_utils_map_contains: \"Was unable to find value inside of map!\"");
	C_UTILS_LOG_INFO(logger, "Destroy Hash Map...");
	MU_ASSERT(c_utils_map_destroy(map, NULL), logger, "c_utils_map_destroy: \"Was unable to destroy map!\"");
	C_UTILS_LOG_INFO(logger, "Success!");
	MU_Logger_destroy(logger);
	return 0;
}