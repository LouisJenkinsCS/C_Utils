#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "PBQueue.h"
volatile int global_var = 1;

void *producer_thread(void *args){
	PBQueue *queue = args;
	int *i = malloc(sizeof(int));
	int *j = malloc(sizeof(int));
	*i = 0;
	*j = 1000000;
	while(global_var){
		PBQueue_Enqueue(queue, i);
		PBQueue_Enqueue(queue, j);
		printf("Pushed value: %d and %d\n", *i, *j);
		(*i)++;
		(*j)--;
	}
	return NULL;
}

void *consumer_thread(void *args){
	PBQueue *queue = args;
	int *i = NULL;
	while(global_var){
		i = PBQueue_Dequeue(queue);
		printf("Popped value: %d\n", *i);
	}
	return NULL;
}

int compare_integers(void *item_one, void *item_two){
	return *((int *)item_one) - *((int *)item_two);
}

int main(void){
	PBQueue *queue = PBQueue_Create_Bounded(2, compare_integers);
	pthread_t** threads = malloc(sizeof(pthread_t) * 2);
	threads[0] = malloc(sizeof(pthread_t));
	threads[1] = malloc(sizeof(pthread_t));
	pthread_create(threads[0], NULL, producer_thread, queue);
	pthread_create(threads[1], NULL, consumer_thread, queue);
	sleep(10);
	global_var = 0;
	void *retval = NULL;
	pthread_join(*(threads[0]), &retval);
	pthread_join(*(threads[1]), &retval);
	printf("Finished!\n");
	return EXIT_SUCCESS;
}