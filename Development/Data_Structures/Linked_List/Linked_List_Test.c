#include <Linked_List.h>
#include <stdlib.h>
#include <Misc_Utils.h>
#include <stdio.h>

static void set_to_zero(void *args){
	*(int *)args = 0;
}

int main(void){
	FILE *fp = fopen("Linked_List_Test_Log.txt", "w");
	Timer_t *timer = Timer_Init(1);
	const int runs = 10000;
	Linked_List *list = Linked_List_create();
	int **array = malloc(sizeof(int *) * runs);
	int i = 0;
	MU_LOG_INFO(fp, "Testing adding elements unsorted...\n");
	for(;i<runs;i++){
		array[i] = malloc(sizeof(int));
		*array[i] = i + runs;
		Linked_List_add(list, array[i], NULL);
	}
	MU_ASSERT(list->size == runs, fp);
	MU_LOG_INFO(fp, "Test passed!\n");
	MU_LOG_INFO(fp, "Testing retrieval of elements at requested index...\n");
	for(i = 0; i<runs;i++){
		int *item = Linked_List_get_at(list, i);
		if(item != array[i]) MU_DEBUG("Item: %d; Array Value: %d\n", *item, *array[i]);
		MU_ASSERT(item == array[i], fp);
	}
	MU_LOG_INFO(fp, "Test passed!\n");
	MU_LOG_INFO(fp, "Testing removal of item with the item as the key and iterator.\n");
	MU_ASSERT(Linked_List_remove_item(list, array[runs/2], NULL), fp);
	int *item = NULL;
	Linked_List_head(list);
	while(item = Linked_List_next(list)) MU_ASSERT(item != array[(runs/2)], fp);
	MU_LOG_INFO(fp, "Test passed!\n");
	MU_LOG_INFO(fp, "Testing removal of elements at requested index...\n");
	for(i = 0; i<(runs-1);i++){
		int *result = Linked_List_remove_at(list, 0, set_to_zero);
		MU_ASSERT(result, fp);
		MU_ASSERT(*result == 0, fp);
	}
	MU_ASSERT(list->size == 0, fp);
	Linked_List_destroy(list, NULL);
	MU_LOG_INFO(fp, "All Tests Passed!\n");
	for(i = 0; i < runs; i++) free(array[i]);
	free(array);
	Timer_Stop(timer);
	char *total_time = Timer_To_String(timer);
	MU_LOG_INFO(fp, "Amount of Runs: %d; Total time is: %s\n", runs, total_time);
	free(total_time);
	Timer_Destroy(timer); 
	fclose(fp);
	return EXIT_SUCCESS;
}