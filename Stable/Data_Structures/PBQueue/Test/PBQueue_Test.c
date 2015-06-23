#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <PBQueue.h>
#include <Misc_Utils.h>

volatile int global_var = 1;
const int bounds = 5;

MU_Logger_t *logger;
PBQueue *queue;

void print_value(void *number){
	MU_LOG_VERBOSE(logger, "Queue has value: %d\n", *(int *)number);
	free(number);
}

void *producer_timeout_thread(void *args){
	int *i = malloc(sizeof(int));
	*i = 0;
	MU_LOG_VERBOSE(logger, "Producer: sleeping for 10 seconds...\n");
	sleep(10);
	MU_LOG_VERBOSE(logger, "Producer: woke up, Enqueueing now!\n");
	PBQueue_Timed_Enqueue(queue, i, 5);
	MU_LOG_VERBOSE(logger, "Producer: sleeping for 10 seconds...\n");
	sleep(10);
	MU_LOG_VERBOSE(logger, "Producer: woke up, timed enqueueing %d times for 10 second timeout!\n", bounds);
	int j = 0;
	for(;j <= bounds; j++){
		MU_LOG_VERBOSE(logger, "Producer: starting Enqueue %d\n", j);
		PBQueue_Timed_Enqueue(queue, i, 10);
	}
	free(i);
	MU_LOG_VERBOSE(logger, "Producer: returning...\n");
}

void *consumer_timeout_thread(void *args){
	int *i = NULL;
	MU_LOG_VERBOSE(logger, "COnsumer: Timed Dequeueing for 5 second timeout!\n");
	if((i = PBQueue_Timed_Dequeue(queue, 5)) == NULL){
		MU_LOG_VERBOSE(logger, "Consumer: Successfully returned Null!\n");
	}
	MU_LOG_VERBOSE(logger, "Consumer: Timed Dequeueing for 10 second timeout!\n");
	if((i = PBQueue_Timed_Dequeue(queue, 10))){
		MU_LOG_VERBOSE(logger, "Consumer: Successfully returned %d\n", *i);
	}
	MU_LOG_VERBOSE(logger, "Consumer: returning...\n");
	return NULL;
}

void *producer_thread(void *args){
	int **i = malloc(sizeof(int *));
	int j = 0;
	while(global_var){
		if(j % bounds == 0) sleep(3);
		i[j] = malloc(sizeof(int));
		*(i[j]) = j;
		PBQueue_Enqueue(queue, i[j]);
		MU_LOG_VERBOSE(logger, "Producer: pushed value: %d\n", *(i[j]));
		j++;
		i = realloc(i, sizeof(int *) * (j+1));
	}
	free(i);
	MU_LOG_VERBOSE(logger, "Producer: returning...\n");
	return NULL;
}

void *consumer_thread(void *args){
	PBQueue *queue = args;
	int *i = NULL;
	while(global_var){
		sleep(1);
		i = PBQueue_Dequeue(queue);
		MU_LOG_VERBOSE(logger, "Consumer: popped value: %d\n", *i);
		free(i);
	}
	MU_LOG_VERBOSE(logger, "Consumer: returning...\n");
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
	MU_LOG_INFO(logger, "Finished Enqueue_and_Dequeue!\n");
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
	MU_LOG_VERBOSE(logger, "Finished Timed_Enqueue_and_Dequeue!\n");
}

int main(void){
	logger = malloc(sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "PBQueue_Test_Log.txt", "w", MU_ALL);
	queue = PBQueue_Create_Bounded(bounds, compare_integers);
	test_enqueue_and_dequeue();
	PBQueue_Clear(queue, print_value);
	test_timed_enqueue_and_dequeue();
	PBQueue_Destroy(queue, NULL);
	MU_LOG_VERBOSE(logger, "Finished All Tests!\n");
	MU_Logger_Destroy(logger, 1);
	return EXIT_SUCCESS;
}