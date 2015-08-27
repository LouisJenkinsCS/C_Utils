#include <DS_Stack.h>
#include <MU_Logger.h>

static MU_Logger_t *logger = NULL;

int main(void){
	logger = MU_Logger_create("./Misc_Utils/Logs/DS_Stack_Test.log", "w", MU_ALL);
	DS_Stack_t *stack = DS_Stack_create();
	for(int i = 0; i < 100; i++){
		int *j = malloc(sizeof(int));
		*j = i;
		DS_Stack_push(stack, j);
		MU_DEBUG("Pushed Val: %d", *j);
	}
	for(int i = 0; i < 100; i++){
		MU_DEBUG("Popped Val: %d", *(int *)DS_Stack_pop(stack));
	}
	return 0;
}