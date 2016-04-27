#define NO_C_UTILS_PREFIX
#include "../map.h"
#include "../../io/logger.h"
#include "../../string/string_buffer.h"
#include <string.h>

static struct c_utils_logger *logger = NULL;
const int buckets = 31;
const int synchronized = 0;

int main(void) {
	logger = logger_create("./data_structures/logs/map_test.log", "w", LOG_LEVEL_ALL);
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

	LOG_VERBOSE(logger, "Logging all Key-Value pairs!");
	
	string_buffer_t *buf = string_buffer_create("{ \n", false);
	for (int i = 0; i < 6; i++)
		STRING_BUFFER_APPEND_FORMAT(buf, "%s: %s\n", keys[i], values[i]);
	string_buffer_append(buf, "}");

	char *str = string_buffer_get(buf);
	LOG_VERBOSE(logger, str);
	free(str);

	LOG_INFO(logger, "Creating Hash Map...");
	map_conf_t conf =
	{
		.logger = logger,
		.callbacks =
		{
			.comparators =
			{
				.value = (void *)strcmp
			}
		}
	};

	map_t *map = map_create_conf(&conf);
	ASSERT(map, logger, "c_utils_map_create: \"Was unable to allocate hash map!\"");
	
	LOG_INFO(logger, "Adding all key-value pairs to hash map...");
	for (int i = 0; i < 6; i++) {
		int was_added = map_add(map, keys[i], values[i]);
		ASSERT(was_added, logger, "c_utils_map_add: \"Was unable to add key: \"%s\" with value: \"%s\"!\"", keys[i], values[i]);
	}
	
	LOG_INFO(logger, "Printing all Key-Value pairs from hash map!");
	char *key, *value;
	C_UTILS_MAP_FOR_EACH_PAIR(key, value, map)
		DEBUG("%s: %s", key, value);
	
	LOG_INFO(logger, "Retrieivng all values from keys from hash map...");
	for (size_t i = 8 - 1; i > 0; i--) {
		char *value_retrieved = map_get(map, keys[i]);
		ASSERT(value_retrieved && strcmp(value_retrieved, values[i]) == 0, logger,
			"c_utils_map_get: \"Was unable to retrieve the right value from key: \"%s\";Expected: \"%s\", but received \"%s\"!\"",
			keys[i], values[i], value_retrieved ? value_retrieved : "NULL");
	}

	LOG_INFO(logger, "Removing Key-Value pair (\"%s\" : \"%s\")...", keys[3], values[3]);
	ASSERT(map_remove(map, keys[3]), logger, "c_utils_map_remove: \"Was unable to remove key: \"%s\"!\"", keys[3]);
	
	LOG_INFO(logger, "Testing for removal of Key-Value pair...");
	void *was_found = map_get(map, keys[3]);
	ASSERT(!was_found, logger, "c_utils_map_get: \"Was unable to correctly remove key: \"%s\"!", keys[3]);
	
	LOG_INFO(logger, "Testing for adding duplicate key...");
	ASSERT(!map_add(map, keys[2], values[2]), logger, "c_utils_map_add: \"Was unable to prevent duplicate key: \"%s\" and value: \"%s\"!\"",
		keys[2], values[2]);
	
	LOG_INFO(logger, "Testing retrieval of key by value...");
	ASSERT(strcmp(keys[1], c_utils_map_contains(map, values[1])) == 0, logger, "c_utils_map_contains: \"Was unable to find value inside of map!\"");
	
	LOG_INFO(logger, "Destroy Hash Map...");
	
	LOG_INFO(logger, "Success!");
	logger_destroy(logger);
	
	return 0;
}