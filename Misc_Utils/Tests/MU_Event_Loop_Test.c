#include <MU_Event_Loop.h>

static const int max_events = 10;

static MU_Event_Loop_t *loop = NULL;

void *prepare_iterator(){
	int *i = malloc(sizeof(int));
	*i = 0;
	MU_DEBUG("Initialized i to %d!", *i);
	return i;
}

void *prepare_timer(){
	static int num = 0;
	int *i = malloc(sizeof(int));
	*i = ++num;
	return i;
}

bool check_iterator(void *args){
	int *i = args;
	return (++(*i) % 1000) == 0;
}

bool dispatch_iterator(void *args){
	int *i = args;
	MU_DEBUG("Incremented i to %d!", *i);
	return false;
}

bool check_string(void *args){
	static int i = 0;
	return (++i % 500) == 0;
}

bool dispatch_string(void *args){
	MU_DEBUG("Hello World!");
	return false;
}

bool dispatch_timer(void *args){
	MU_DEBUG("%d seconds passed!", *(int *)args);
	return false;
}

bool finalize_iterator(void *args){
	int *i = args;
	free(i);
	MU_Event_Loop_stop(loop);
	return true;
}

int main(void){
	int i = 0;
	loop = MU_Event_Loop_create();
	MU_Event_Source_t *events[max_events];
	for(; i < max_events; i++){
		events[i] = MU_Event_Source_create(prepare_timer, NULL, dispatch_timer, NULL, (i + 1) * 1000);
		MU_Event_Loop_add(loop, events[i]);
	}
	MU_Event_Loop_run(loop);
	return 0;
}