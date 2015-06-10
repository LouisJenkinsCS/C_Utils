#include <Linked_List.h>
#include <stdlib.h>
#include <Misc_Utils.h>
#include <stdio.h>


static MU_Logger_t *logger = NULL;

static void set_to_zero(void *item){
	*(int *)item = 0;
}

static int compare_ints(void *item_one, void *item_two){
	return (*(int *)item_one - *(int *)item_two);
}

static int inverse_compare_ints(void *item_one, void *item_two){
	return (*(int *)item_two - *(int *)item_one);
}

static char *print_item(void *item){
	char *item_to_string;
	asprintf(&item_to_string, "%d", *(int *)item);
	return item_to_string;
}

int main(void){
	srand(time(NULL));
	logger = calloc(1, sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "Linked_List_Test_Log.txt", "w", MU_ALL);
	Timer_t *timer = Timer_Init(1);
	const int runs = 10000;
	Linked_List_t *list = Linked_List_create();
	void **array = malloc(sizeof(int *) * runs);
	int i = 0;
	MU_LOG_INFO(logger, "Testing adding elements unsorted...\n");
	for(;i<runs;i++){
		array[i] = malloc(sizeof(int));
		*(int *)array[i] = i * (rand() % runs);
		Linked_List_add(list, array[i], NULL);
	}
	MU_ASSERT(Linked_List_get_at(list, runs/2) == array[runs/2], logger);
	MU_ASSERT(list->size == runs, logger);
	MU_LOG_INFO(logger, "Test passed!\n");
	MU_LOG_INFO(logger, "Testing retrieval of elements at requested index...\n");
	Linked_List_head(list);
	for(i = 1; i<runs;i++) MU_ASSERT(Linked_List_next(list) == array[i], logger);
	MU_LOG_INFO(logger, "Test passed!\n");
	MU_LOG_INFO(logger, "Testing removal of item with the item as the key and iterator.\n");
	MU_ASSERT(Linked_List_remove_item(list, array[runs/2], NULL), logger);
	int *item = NULL;
	Linked_List_head(list);
	while(item = Linked_List_next(list)) MU_ASSERT(item != array[(runs/2)], logger);
	MU_LOG_INFO(logger, "Test passed!\n");
	MU_LOG_VERBOSE(logger, "Printing all elements inside of list unsorted!\n");
	Linked_List_print_all(list, logger->file, print_item);
	MU_LOG_INFO(logger, "Testing removal of elements at requested index...\n");
	for(i = 0; i<(runs-1);i++){
		int *result = Linked_List_remove_at(list, 0, NULL);
		MU_ASSERT(result, logger);
	}
	MU_ASSERT(list->size == 0, logger);
	MU_LOG_INFO(logger, "Test passed!\n");
	MU_LOG_INFO(logger, "Testing the Array-To-Linked_List functionality and sorting!\n");
	Linked_List_t *list_two = Linked_List_create_from(array, runs, inverse_compare_ints);
	MU_LOG_INFO(logger, "Printing all elements inside of list_two in descending order!\n");
	Linked_List_print_all(list_two, logger->file, print_item);
	Linked_List_tail(list_two);
	size_t size = 0;
	void **sorted_array = Linked_List_to_array(list_two, &size);
	for(i = 1; i > size; i--) {
		void *item = Linked_List_previous(list_two);
		if(item != sorted_array[i]) MU_DEBUG("Iteration: %d: %d != %d\n", i,  *(int *)item, *(int *)array[i]);
		MU_ASSERT(item == sorted_array[i], logger);
	}
	Linked_List_sort(list_two, compare_ints);
	for(i = 0;i < runs; i += 2) {
		void *result_one = Linked_List_next(list_two);
		void *result_two = Linked_List_next(list_two);
		if(!result_one || !result_two) break;
		MU_ASSERT((*(int *)(result_one)) <= (*(int *)(result_two)), logger);
	}
	MU_LOG_VERBOSE(logger, "Printing all elements inside of list_two in ascending order!\n");
	Linked_List_print_all(list_two, logger->file, print_item);
	Linked_List_destroy(list, NULL);
	Linked_List_destroy(list_two, free);
	free(array);
	free(sorted_array);
	Timer_Stop(timer);
	char *total_time = Timer_To_String(timer);
	MU_LOG_INFO(logger, "All Tests Passed!\n");
	MU_LOG_INFO(logger, "Amount of Runs: %d; Total time is: %s\n", runs, total_time);
	free(total_time);
	Timer_Destroy(timer); 
	MU_Logger_Deref(logger, 1);
	return EXIT_SUCCESS;
}