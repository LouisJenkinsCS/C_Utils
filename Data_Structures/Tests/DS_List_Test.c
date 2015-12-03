#include <DS_List.h>
#include <stdlib.h>
#include <MU_Logger.h>
#include <stdio.h>


static MU_Logger_t *logger = NULL;

static int compare_ints(const void *item_one, const void *item_two){
	return (*(int *)item_one - *(int *)item_two);
}

static int inverse_compare_ints(const void *item_one, const void *item_two){
	return (*(int *)item_two - *(int *)item_one);
}

static char *print_item(const void *item){
	char *item_to_string;
	if(!item) asprintf(&item_to_string, "%s", "NULL");
	else asprintf(&item_to_string, "%d", *(int *)item);
	return item_to_string;
}

int main(void){
	srand(time(NULL));
	logger = MU_Logger_create("./Data_Structures/Logs/DS_List_Test.log", "w", MU_ALL);
	const int runs = 1000;
	DS_List_t *list = DS_List_create(true);
	void **array = malloc(sizeof(int *) * runs);
	int i = 0;
	MU_LOG_INFO(logger, "Testing adding elements unsorted...\n");
	for(;i<runs;i++){
		array[i] = malloc(sizeof(int));
		*(int *)array[i] = i * (rand() % runs);
		DS_List_add(list, array[i], NULL);
	}
	DS_Iterator_t *it = DS_List_iterator(list);
	MU_ASSERT(*(int *)DS_List_get(list, runs/2) == *(int *)array[runs/2], logger, "Unable to get the right element from the list!");
	MU_ASSERT(list->size == runs, logger, "List size inaccurate!");
	MU_LOG_INFO(logger, "Testing retrieval of elements at requested index...\n");
	for(i = 0; i<runs;i++) MU_ASSERT((*(int *)DS_Iterator_next(it)) == *(int *)array[i], logger, "List iterator at invalid entry!");
	MU_LOG_INFO(logger, "Testing removal of item with the item as the key and iterator.\n");
	MU_ASSERT(DS_List_remove_item(list, array[runs/2], NULL), logger, "Unable to remove item from the list!");
	int *item = NULL;
	while((item = DS_Iterator_next(it))) MU_ASSERT((*item != (*(int *)array[(runs/2)])), logger, "List iterator has outdated elements! \
		item in list: %d;item in array: %d", *item, *(int *)array[(runs/2)]);
	MU_LOG_VERBOSE(logger, "Printing all elements inside of list unsorted!\n");
	DS_List_print_all(list, logger->file, print_item);
	MU_LOG_INFO(logger, "Testing removal of elements at requested index...\n");
	for(i = 0; i<(runs-1);i++){
		int *result = DS_List_remove_at(list, 0, NULL);
		MU_ASSERT(result, logger, "Was unable to remove an element!");
	}
	MU_ASSERT(list->size == 0, logger, "List's size was not properly decremented!");
	MU_LOG_INFO(logger, "Testing the Array-To-DS_List functionality and sorting!\n");
	DS_List_t *list_two = DS_List_create_from(array, runs, inverse_compare_ints, false);
	free(it);
	it = DS_List_iterator(list_two);
	MU_LOG_INFO(logger, "Printing all elements inside of list_two in descending order!\n");
	DS_List_print_all(list_two, logger->file, print_item);
	size_t size = 0;
	void **sorted_array = DS_List_to_array(list_two, &size);
	for(i = 1; i > size; i--) {
		void *item = DS_Iterator_prev(it);
		if(item != sorted_array[i]) MU_DEBUG("Iteration: %d: %d != %d\n", i,  *(int *)item, *(int *)array[i]);
		MU_ASSERT(*(int *)item == *(int *)sorted_array[i], logger, "Array returned is inaccurate to list!");
	}
	DS_List_sort(list_two, compare_ints);
	for(i = 0;i < runs; i += 2) {
		void *result_one = DS_Iterator_next(it);
		void *result_two = DS_Iterator_next(it);
		if(!result_one || !result_two) break;
		MU_ASSERT(((*(int *)(result_one)) <= (*(int *)(result_two))), logger, "List was improperly sorted!%d <= %d...", *(int *)result_one, *(int *)result_two);
	}
	MU_LOG_VERBOSE(logger, "Printing all elements inside of list_two in ascending order!\n");
	DS_List_print_all(list_two, logger->file, print_item);
	MU_LOG_INFO(logger, "Testing adding elements before and after the current elements!\n");
	DS_Iterator_head(it);
	DS_Iterator_prepend(it, NULL);
	DS_Iterator_append(it, NULL);
	DS_List_print_all(list_two, logger->file, print_item);
	MU_ASSERT(!list_two->head->item && !list_two->head->_double.next->_double.next->item, logger, "Was unable to add after or before!");
	DS_List_destroy(list, NULL);
	DS_List_destroy(list_two, free);
	free(it);
	free(array);
	free(sorted_array);
	MU_LOG_INFO(logger, "All Tests Passed!\n");
	MU_Logger_destroy(logger);
	return EXIT_SUCCESS;
}