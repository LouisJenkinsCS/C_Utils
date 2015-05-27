#include <pthread.h>
#include <stdlib.h>
#include "PBQueue.h"
volatile int global_var = 1;

void *producer_thread(void *args){
	PBQueue *queue = args;
	int *i = malloc(sizeof(int));
	*i = 0;
	while(global_var){
		PBQueue_Enqueue(queue, i);
		printf("Pushed value: %d\n", *i);
		*i++;
	}
}

void *consumer_thread(void *args){
	PBQueue *queue = args;
	int *i = NULL;
	while(global_var){
		i = PBQueue_Dequeue(queue);
		printf("Popped value: %d\n", *i);
		free(i);
	}
}

int compare_integers(void *item_one, void *item_two){
	return *((int *)item_one) - *((int *)item_two);
}

int main(void){
	PBQueue *queue = PBQueue_Create_Bounded(1, compare_integers);
	pthread_t** threads = malloc(sizeof(pthread_t) * 2);
	pthread_create(threads[0], NULL, producer_thread, queue);
	pthread_create(threads[1], NULL, consumer_thread, queue);
	sleep(10);
	global_var = 0;
	pthread_join(threads[0]);
	pthread_join(threads[1]);
}