#include <Linked_List.h>
#include <stdlib.h>
#include <Misc_Utils.h>
#include <stdio.h>

static MU_Logger_t *logger = NULL;

static void set_to_zero(void *item){
	*(int *)item = 0;
}

static int compare_ints(void *item_one, void *item_two){
	return (*(int *)item_two - *(int *)item_one);
}

static void print_item(void *item){
	MU_LOG_VERBOSE(logger, "Val: %d\n", *(int *)item);
}

int main(void){
	logger = calloc(1, sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "Linked_List_Test_Log.txt", "w", MU_ALL);
	Timer_t *timer = Timer_Init(1);
	const int runs = 100;
	Linked_List *list = Linked_List_create();
	void **array = malloc(sizeof(int *) * runs);
	int i = 0;
	MU_LOG_INFO(logger, "Testing adding elements unsorted...\n");
	for(;i<runs;i++){
		array[i] = malloc(sizeof(int));
		*(int *)array[i] = i + runs;
		Linked_List_add(list, array[i], NULL);
	}
	MU_ASSERT(list->size == runs, logger);
	MU_LOG_INFO(logger, "Test passed!\n");
	MU_LOG_INFO(logger, "Testing retrieval of elements at requested index...\n");
	for(i = 0; i<runs;i++){
		// Warning extremely slow! Exponential increase in runtime the higher runs gets.
		MU_ASSERT(Linked_List_get_at(list, i) == array[i], logger);
	}
	MU_LOG_INFO(logger, "Test passed!\n");
	MU_LOG_INFO(logger, "Testing removal of item with the item as the key and iterator.\n");
	MU_ASSERT(Linked_List_remove_item(list, array[runs/2], NULL), logger);
	int *item = NULL;
	Linked_List_head(list);
	while(item = Linked_List_next(list)) MU_ASSERT(item != array[(runs/2)], logger);
	MU_LOG_INFO(logger, "Test passed!\n");
	MU_LOG_VERBOSE(logger, "Printing all elements inside of list!\n");
	Linked_List_for_each(list, print_item);
	MU_LOG_INFO(logger, "Testing removal of elements at requested index...\n");
	for(i = 0; i<(runs-1);i++){
		int *result = Linked_List_remove_at(list, 0, NULL);
		MU_ASSERT(result, logger);
		//MU_ASSERT(*result == 0, logger);
	}
	MU_ASSERT(list->size == 0, logger);
	MU_LOG_INFO(logger, "Test passed!\n");
	MU_LOG_INFO(logger, "Testing the Array-To-Linked_List functionality and sorting!\n");
	Linked_List *list_two = Linked_List_create_from(array, runs);
	Linked_List_sort(list_two, compare_ints);
	for(i = 0;i < runs; i += 2) MU_ASSERT(Linked_List_next(list_two) <= Linked_List_next(list_two), logger);
	MU_LOG_VERBOSE(logger, "Printing all elements inside of list_two!\n");
	Linked_List_for_each(list_two, print_item);
	Linked_List_destroy(list, NULL);
	Linked_List_destroy(list_two, NULL);
	MU_DEBUG("compare(%d, %d) = %d\n", *(int *)array[1], *(int *)array[2], compare_ints(array[0], array[1]));
	free(array);
	Timer_Stop(timer);
	char *total_time = Timer_To_String(timer);
	MU_LOG_INFO(logger, "All Tests Passed!\n");
	MU_LOG_INFO(logger, "Amount of Runs: %d; Total time is: %s\n", runs, total_time);
	free(total_time);
	Timer_Destroy(timer); 
	MU_Logger_Deref(logger, 1);
	return EXIT_SUCCESS;
}