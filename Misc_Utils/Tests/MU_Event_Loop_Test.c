#include <MU_Event_Loop.h>

void *prepare_iterator(){
	int *i = malloc(sizeof(int));
	*i = 0;
	MU_DEBUG("Initialized i to %d!", *i);
	return i;
}

bool dispatch_iterator(void *args){
	int *i = args;
	if((*i)++ % 100) return true;
	MU_DEBUG("Incremented i to %d!", *i);
	return false;
}

bool finalize_iterator(void *args){
	int *i = args;
	free(i);
	return true;
}

int main(void){
	MU_Event_Source_t *source = MU_Event_Source_create(prepare_iterator, dispatch_iterator, finalize_iterator);
	MU_Event_Loop_t *loop = MU_Event_Loop_create();
	MU_Event_Loop_add(loop, source);
	MU_Event_Loop_run(loop);
	return 0;
}