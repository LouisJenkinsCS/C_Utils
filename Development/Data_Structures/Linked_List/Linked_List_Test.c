#include <Linked_List.h>
#include <stdlib.h>
#include <Misc_Utils.h>
#include <stdio.h>

static void set_to_zero(void *args){
	*(int *)args = 0;
}

int main(void){
	FILE *fp = fopen("Linked_List_Test_Log.txt", "w");
	const int runs = 100;
	Linked_List *list = Linked_List_create();
	int **array = malloc(sizeof(int *) * runs);
	int i = 0;
	for(;i<runs;i++){
		array[i] = malloc(sizeof(int));
		*array[i] = i + runs;
		Linked_List_add(list, array[i], NULL);
	}
	MU_ASSERT(list->size == runs, fp)
	for(i = 0; i<runs;i++){
		MU_ASSERT(Linked_List_get_at(list, i) == array[i], fp);
	}
	for(i = 0; i<runs;i++){
		Linked_List_remove_at(list, i, set_to_zero);
		MU_ASSERT(*array[i] == 0, fp);
	}
	MU_ASSERT(list->size == 0, fp);
	Linked_List_destroy(list, NULL);
	fclose(fp);
	return EXIT_SUCCESS;
}