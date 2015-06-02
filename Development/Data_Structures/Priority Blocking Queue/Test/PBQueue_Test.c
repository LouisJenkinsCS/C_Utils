#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "PBQueue.h"
volatile int global_var = 1;
const int bounds = 5;

PBQueue *queue;

void print_value(void *number){
	printf("Queue has value: %d\n", *(int *)number);
	free(number);
}

void *producer_timeout_thread(void *args){
	PBQueue *queue = args;
	int *i = malloc(sizeof(int));
	*i = 0;
	printf("Producer: sleeping for 10 seconds...\n");
	sleep(10);
	printf("Producer: woke up, Enqueueing now!\n");
	PBQueue_Timed_Enqueue(queue, i, 5);
	printf("Producer: sleeping for 10 seconds...\n");
	sleep(10);
	printf("Producer: woke up, timed enqueueing %d times for 10 second timeout!\n", bounds);
	int j = 0;
	for(;j <= bounds; j++){
		printf("Producer: starting Enqueue %d\n", j);
		PBQueue_Timed_Enqueue(queue, i, 10);
	}
	free(i);
	printf("Producer: returning...\n");
}

void *consumer_timeout_thread(void *args){
	PBQueue *queue = args;
	int *i = NULL;
	printf("COnsumer: Timed Dequeueing for 5 second timeout!\n");
	if((i = PBQueue_Timed_Dequeue(queue, 5)) == NULL){
		printf("Consumer: Successfully returned Null!\n");
	}
	printf("Consumer: Timed Dequeueing for 10 second timeout!\n");
	if((i = PBQueue_Timed_Dequeue(queue, 10))){
		printf("Consumer: Successfully returned %d\n", *i);
	}
	printf("Consumer: returning...\n");
	return NULL;
}

void *producer_thread(void *args){
	PBQueue *queue = args;
	int **i = malloc(sizeof(int *));
	int j = 0;
	while(global_var){
		if(j % bounds == 0) sleep(3);
		i[j] = malloc(sizeof(int));
		*(i[j]) = j;
		PBQueue_Enqueue(queue, i[j]);
		printf("Pushed value: %d\n", *(i[j]));
		j++;
		i = realloc(i, sizeof(int *) * (j+1));
	}
	free(i);
	printf("Producer returning...\n");
	return NULL;
}

void *consumer_thread(void *args){
	PBQueue *queue = args;
	int *i = NULL;
	while(global_var){
		sleep(1);
		i = PBQueue_Dequeue(queue);
		printf("Popped value: %d\n", *i);
		free(i);
	}
	printf("Consumer returning...\n");
	return NULL;
}

int compare_integers(void *item_one, void *item_two){
	return *((int *)item_one) - *((int *)item_two);
}

void test_enqueue_and_dequeue(void){
	pthread_t** threads = malloc(sizeof(pthread_t) * 2);
	threads[0] = malloc(sizeof(pthread_t));
	threads[1] = malloc(sizeof(pthread_t));
	pthread_create(threads[0], NULL, producer_thread, queue);
	pthread_create(threads[1], NULL, consumer_thread, queue);
	sleep(30);
	global_var = 0;
	void *retval = NULL;
	pthread_join(*(threads[0]), &retval);
	pthread_join(*(threads[1]), &retval);
	free(threads[0]);
	free(threads[1]);
	free(threads);
	printf("Finished Enqueue_and_Dequeue!\n");
}

void test_timed_enqueue_and_dequeue(void){
	pthread_t** threads = malloc(sizeof(pthread_t) * 2);
	threads[0] = malloc(sizeof(pthread_t));
	threads[1] = malloc(sizeof(pthread_t));
	pthread_create(threads[0], NULL, producer_timeout_thread, queue);
	pthread_create(threads[1], NULL, consumer_timeout_thread, queue);
	sleep(30);
	void *retval = NULL;
	pthread_join(*(threads[0]), &retval);
	pthread_join(*(threads[1]), &retval);
	free(threads[0]);
	free(threads[1]);
	free(threads);
	printf("Finished Timed_Enqueue_and_Dequeue!\n");
}

int main(void){
	queue = PBQueue_Create_Bounded(bounds, compare_integers);
	test_enqueue_and_dequeue();
	PBQueue_Clear(queue, print_value);
	test_timed_enqueue_and_dequeue();
	PBQueue_Destroy(queue, NULL);
	printf("Finished All Tests!\n");
	return EXIT_SUCCESS;
}