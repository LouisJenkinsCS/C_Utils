#define NO_C_UTILS_PREFIX

#include "../heap.h"
#include "../../io/logger.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define C_UTILS_HEAP_TEST_MAX_SIZE 1000

static int compare_ints(const void *f, const void *s) {
	return *(int *)f - *(int *)s;
}

static logger_t *logger;

LOGGER_AUTO_CREATE(logger, "data_structures/logs/heap_test.log", "w", LOG_LEVEL_ALL);

int main(void) {
	heap_conf_t conf =
	{
		.logger = logger
	};

	heap_t *heap = heap_create_conf(compare_ints, &conf);
	int arr[C_UTILS_HEAP_TEST_MAX_SIZE] = {0};

	srand(time(NULL));

	for(int i = 0; i < C_UTILS_HEAP_TEST_MAX_SIZE; i++) {
		arr[i] = rand() % 100 + 1;
		heap_insert(heap, arr + i);
	}

	int last = 0;
	for(int i = 0; i < C_UTILS_HEAP_TEST_MAX_SIZE; i++) {
		void *item = heap_remove(heap);
		if(!item)
			break;

		int curr = *(int *) item;
		if(last)
			assert(curr <= last);

		last = curr;

		C_UTILS_DEBUG("%d\n", curr);
	}

	heap_destroy(heap);
}