#include <DS_Queue.h>
#include <MU_Logger.h>
#include <TP_Pool.h>
#include <unistd.h>

static const int num_threads = 4;

static DS_Queue_t *queue = NULL;

static MU_Logger_t *logger = NULL;

volatile long long int counter = 250000LL;

static void *enqueue_to_queue(void *args){
	while(counter >= 0){
		int *i = malloc(sizeof(int));
		*i = __sync_sub_and_fetch(&counter, 1);
		DS_Queue_enqueue(queue, i);
	}
	return NULL;
}

static void *dequeue_from_queue(void *args){
	while(true){
		int *i = DS_Queue_dequeue(queue);
		if(!i) continue;
		else if(*i <= 0) break;
		MU_DEBUG("Popped Val: %d", *(int *)i);
		free(i);
	}
	return NULL;
}

int main(void){
	logger = MU_Logger_create("./Data_Structures/Logs/DS_Queue_Test.log", "w", MU_ALL);
	queue = DS_Queue_create();
	TP_Pool_t *tp = TP_Pool_create(num_threads);
	int i = 0;
	for(; i < num_threads; i++){
		TP_Pool_add(tp, (i % 2 == 0) ? enqueue_to_queue : dequeue_from_queue, NULL, TP_NO_RESULT);
	}
	TP_Pool_wait(tp, 300);
	TP_Pool_wait(tp, -1);
	TP_Pool_destroy(tp);
	MU_Logger_destroy(logger);
	DS_Queue_destroy(queue, free);
	return 0;
}