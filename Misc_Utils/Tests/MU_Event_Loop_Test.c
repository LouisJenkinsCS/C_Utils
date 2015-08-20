#include <MU_Event_Loop.h>

MU_Event_Loop_t *loop = NULL;

void *prepare_iterator(){
	int *i = malloc(sizeof(int));
	*i = 0;
	MU_DEBUG("Initialized i to %d!", *i);
	return i;
}

bool dispatch_iterator(void *args){
	int *i = args;
	if((++(*i) % 1000) != 0) return false;
	MU_DEBUG("Incremented i to %d!", *i);
	return false;
}

bool dispatch_string(void *args){
	static int i = 0;
	i++;
	if((i % 500) == 0) MU_DEBUG("Hello World!");
	return false;
}

bool dispatch_short_timer(void *args){
	MU_DEBUG("One Second Ellapsed!");
	return false;
}

bool dispatch_medium_timer(void *args){
	MU_DEBUG("Five Second Ellapsed!");
	return false;
}

bool dispatch_long_timer(void *args){
	MU_DEBUG("Ten Second Ellapsed!");
	return false;
}

bool finalize_iterator(void *args){
	int *i = args;
	free(i);
	MU_Event_Loop_stop(loop);
	return true;
}

int main(void){
	MU_Event_Source_t *source_one = MU_Event_Source_create(prepare_iterator, dispatch_iterator, finalize_iterator, 0);
	MU_Event_Source_t *source_two = MU_Event_Source_create(NULL, dispatch_string, NULL, 0);
	MU_Event_Source_t *timed_source_one = MU_Event_Source_create(NULL, dispatch_short_timer, NULL, 1000);
	MU_Event_Source_t *timed_source_two = MU_Event_Source_create(NULL, dispatch_medium_timer, NULL, 5000);
	MU_Event_Source_t *timed_source_three = MU_Event_Source_create(NULL, dispatch_long_timer, NULL, 10000);
	loop = MU_Event_Loop_create();
	MU_Event_Loop_add(loop, timed_source_two);
	MU_Event_Loop_add(loop, timed_source_one);
	MU_Event_Loop_add(loop, timed_source_three);
	MU_Event_Loop_add(loop, source_one);
	MU_Event_Loop_add(loop, source_two);
	MU_Event_Loop_run(loop);
	return 0;
}