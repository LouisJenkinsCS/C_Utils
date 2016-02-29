#define NO_C_UTILS_PREFIX

#include "../list.h"
#include <stdlib.h>
#include "../../io/logger.h"
#include <stdio.h>


static struct c_utils_logger *logger = NULL;

static int compare_ints(const void *item_one, const void *item_two) {
	return (*(int *)item_one - *(int *)item_two);
}

static int inverse_compare_ints(const void *item_one, const void *item_two) {
	return (*(int *)item_two - *(int *)item_one);
}

static char *print_item(const void *item) {
	char *item_to_string;
	if (!item) asprintf(&item_to_string, "%s", "NULL");
	else asprintf(&item_to_string, "%d", *(int *)item);
	return item_to_string;
}

int main(void) {
	srand(time(NULL));
	logger = c_utils_logger_create("./Data_Structures/Logs/c_utils_list_Test.log", "w", LOG_LEVEL_ALL);
	const int runs = 1000;
	struct c_utils_list *list = c_utils_list_create(true);
	void *tmp;
	C_UTILS_LIST_FOR_EACH(tmp, list) {

	}
	void **array = malloc(sizeof(int *) * runs);
	int i = 0;
	C_UTILS_LOG_INFO(logger, "Testing adding elements unsorted...\n");
	for (;i<runs;i++) {
		array[i] = malloc(sizeof(int));
		*(int *)array[i] = i * (rand() % runs);
		c_utils_list_add(list, array[i], NULL);
	}
	struct c_utils_iterator *it = c_utils_list_iterator(list);
	C_UTILS_ASSERT(*(int *)c_utils_list_get(list, runs/2) == *(int *)array[runs/2], logger, "Unable to get the right element from the list!");
	C_UTILS_ASSERT(c_utils_list_size(list) == runs, logger, "List size inaccurate!");
	C_UTILS_LOG_INFO(logger, "Testing retrieval of elements at requested index...\n");
	for (i = 0; i<runs;i++) C_UTILS_ASSERT((*(int *)c_utils_iterator_next(it)) == *(int *)array[i], logger, "List iterator at invalid entry!");
	C_UTILS_LOG_INFO(logger, "Testing removal of item with the item as the key and iterator.\n");
	C_UTILS_ASSERT(c_utils_list_remove_item(list, array[runs/2], NULL), logger, "Unable to remove item from the list!");
	int *item = NULL;
	while ((item = c_utils_iterator_next(it))) C_UTILS_ASSERT((*item != (*(int *)array[(runs/2)])), logger, "List iterator has outdated elements! \
		item in list: %d;item in array: %d", *item, *(int *)array[(runs/2)]);
	C_UTILS_LOG_VERBOSE(logger, "Printing all elements inside of list unsorted!\n");
	c_utils_list_print_all(list, logger->file, print_item);
	C_UTILS_LOG_INFO(logger, "Testing removal of elements at requested index...\n");
	for (i = 0; i<(runs-1);i++) {
		int *result = c_utils_list_remove_at(list, 0, NULL);
		C_UTILS_ASSERT(result, logger, "Was unable to remove an element!");
	}
	C_UTILS_ASSERT(list->size == 0, logger, "List's size was not properly decremented!");
	C_UTILS_LOG_INFO(logger, "Testing the Array-To-DS_List functionality and sorting!\n");
	struct c_utils_list *list_two = c_utils_list_create_from(array, runs, inverse_compare_ints, false);
	free(it);
	it = c_utils_list_iterator(list_two);
	C_UTILS_LOG_INFO(logger, "Printing all elements inside of list_two in descending order!\n");
	c_utils_list_print_all(list_two, logger->file, print_item);
	size_t size = 0;
	void **sorted_array = c_utils_list_as_array(list_two, &size);
	for (i = 1; i > size; i--) {
		void *item = c_utils_iterator_prev(it);
		if (item != sorted_array[i]) C_UTILS_DEBUG("Iteration: %d: %d != %d\n", i,  *(int *)item, *(int *)array[i]);
		C_UTILS_ASSERT(*(int *)item == *(int *)sorted_array[i], logger, "Array returned is inaccurate to list!");
	}
	c_utils_list_sort(list_two, compare_ints);
	for (i = 0;i < runs; i += 2) {
		void *result_one = c_utils_iterator_next(it);
		void *result_two = c_utils_iterator_next(it);
		if (!result_one || !result_two) break;
		C_UTILS_ASSERT(((*(int *)(result_one)) <= (*(int *)(result_two))), logger, "List was improperly sorted!%d <= %d...", *(int *)result_one, *(int *)result_two);
	}
	C_UTILS_LOG_VERBOSE(logger, "Printing all elements inside of list_two in ascending order!\n");
	c_utils_list_print_all(list_two, logger->file, print_item);
	C_UTILS_LOG_INFO(logger, "Testing adding elements before and after the current elements!\n");
	c_utils_iterator_head(it);
	c_utils_iterator_prepend(it, NULL);
	c_utils_iterator_append(it, NULL);
	c_utils_list_print_all(list_two, logger->file, print_item);
	C_UTILS_ASSERT(!list_two->head->item && !list_two->head->_double.next->_double.next->item, logger, "Was unable to add after or before!");
	c_utils_list_destroy(list, NULL);
	c_utils_list_destroy(list_two, free);
	free(it);
	free(array);
	free(sorted_array);
	C_UTILS_LOG_INFO(logger, "All Tests Passed!\n");
	C_UTILS_Logger_destroy(logger);
	return EXIT_SUCCESS;
}