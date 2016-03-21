#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NO_C_UTILS_PREFIX

#include "../list.h"
#include "../../io/logger.h"
#include "../../string/string_buffer.h"

static const int runs = 100;

static logger_t *logger;

LOGGER_AUTO_CREATE(logger, "./data_structures/logs/list_test.log", "w", LOG_LEVEL_ALL);

static int compare_ints(const void *item_one, const void *item_two) {
	return (*(int *)item_one - *(int *)item_two);
}

static int inverse_compare_ints(const void *item_one, const void *item_two) {
	return (*(int *)item_two - *(int *)item_one);
}

static void print_all(list_t *list) {
	int *item;
	string_buffer_t *buf = string_buffer_create(" { ", false);
	LIST_FOR_EACH(item, list) {
		STRING_BUFFER_APPEND(buf, *item);
		string_buffer_append(buf, ", ");
	}

	// Strip the last ", " from the string buffer.
	string_buffer_delete(buf, STRING_BUFFER_END - 1, STRING_BUFFER_END);

	string_buffer_append(buf, " } ");

	char *all_str = string_buffer_get(buf);
	LOG_INFO(logger, "%s", all_str);

	free(all_str);
}

int main(void) {
	srand(time(NULL));

	list_conf_t conf =
	{
		.logger = logger,
		.flags = LIST_RC_INSTANCE,
		.callbacks =
		{
			.comparator = inverse_compare_ints,
			.destructor = free
		},
	};
	void **array = malloc(sizeof(int *) * runs);
	list_t *list = list_create(&conf);

	LOG_INFO(logger, "Testing adding elements unsorted...\n");
	int i = 0;
	for (; i < runs; i++) {
		array[i] = malloc(sizeof(int));
		*(int *)array[i] = i * (rand() % runs);
		list_add(list, array[i]);
	}

	// Testing that insertion worked. List should be an exact copy of array.
	int middle = runs/2;
	ASSERT((*(int *) list_get(list, middle)) == (*(int *) array[middle])
		, logger, "List and Array have different items or order than it was inserted!");

	// Since we inserted runs amount of elements, the size should be the same too.
	ASSERT(c_utils_list_size(list) == runs, logger, "List size inaccurate!");

	LOG_INFO(logger, "Comparing elements of both array and list...\n");
	iterator_t *it = list_iterator(list);
	for (i = 0; i<runs;i++) 
		ASSERT((*(int *)c_utils_iterator_next(it)) == (*(int *)array[i]),
			logger, "Iterator failed on comparison of elements!");
	
	LOG_INFO(logger, "Testing removal of item from list.\n");
	list_remove(list, array[middle]);
	
	// We test both the iterator and list by seeing if we still find the removed element in list.
	int *item = NULL;
	while ((item = c_utils_iterator_next(it))) 
		ASSERT((*item != (*(int *)array[middle])),
			logger, "List iterator has outdated elements! Item in list: %d; Item in array: %d",
			*item, *(int *)array[middle]);
	
	LOG_VERBOSE(logger, "Printing all elements inside of list unsorted!\n");
	print_all(list);

	LOG_INFO(logger, "Testing removal of elements at requested index...\n");
	for (i = 0; i<(runs-1);i++) {
		int *result = list_remove_at(list, 0);
		ASSERT(result, logger, "Was unable to remove an element!");
	}

	// Testing that the list is reportedly empty.
	ASSERT(list_size(list) == 0, logger, "List's size was not properly managed!");

	LOG_INFO(logger, "Creating a list from the array and sorting!\n");
	conf.flags |= LIST_DELETE_ON_DESTROY;
	conf.callbacks.comparator = compare_ints;
	list_t *list_two = list_from_conf(array, runs, &conf);

	iterator_destroy(it);
	it = list_iterator(list_two);
	
	LOG_INFO(logger, "Printing all elements inside of list_two in descending order!\n");
	print_all(list_two);
	
	size_t size = 0;
	void **sorted_array = list_as_array(list_two, &size);
	for (i = 1; i > size; i--) {
		void *item = iterator_prev(it);
		if (item != sorted_array[i]) DEBUG("Iteration: %d: %d != %d\n", i,  *(int *)item, *(int *)array[i]);
		ASSERT(*(int *)item == *(int *)sorted_array[i], logger, "Array returned is inaccurate to list!");
	}

	for (i = 0;i < runs; i += 2) {
		void *result_one = iterator_next(it);
		void *result_two = iterator_next(it);
		if (!result_one || !result_two) 
			break;
		ASSERT(((*(int *)(result_one)) <= (*(int *)(result_two))),
			logger, "List was improperly sorted!%d <= %d...", *(int *)result_one, *(int *)result_two);
	}

	LOG_VERBOSE(logger, "Printing all elements inside of list_two in ascending order!\n");
	print_all(list_two);
	
	LOG_INFO(logger, "Testing adding elements before and after the current elements!\n");
	iterator_head(it);
	iterator_prepend(it, NULL);
	iterator_append(it, NULL);

	LOG_VERBOSE(logger, "Printing all elements inside of list_two after inserting bad elements!\n");
	print_all(list_two);

	LOG_INFO(logger, "Clearing the list through the iterator...");
	LIST_FOR_EACH_REV(item, list_two)
		iterator_delete(_this_iterator);

	LOG_VERBOSE(logger, "Printing all elements inside of list_two after clearing through iterator!\n");
	print_all(list_two);

	list_destroy(list);
	list_destroy(list_two);
	iterator_destroy(it);
	free(array);
	free(sorted_array);

	LOG_INFO(logger, "All Tests Passed!\n");
	return EXIT_SUCCESS;
}