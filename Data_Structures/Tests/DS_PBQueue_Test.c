#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <DS_PBQueue.h>
#include <MU_Logger.h>

static volatile int global_var = 1;
static const int max_size = 5;
static const int max_num = 10;
static const int sleep_after = 5;
static int arr[10];

static MU_Logger_t *logger;
static DS_PBQueue_t *queue;

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
	DS_PBQueue_enqueue(queue, i, 5);
	MU_LOG_VERBOSE(logger, "Producer: sleeping for 10 seconds...\n");
	sleep(10);
	MU_LOG_VERBOSE(logger, "Producer: woke up, timed enqueueing %d times for 10 second timeout!\n", max_size);
	int j = 0;
	for(;j <= max_size; j++){
		MU_LOG_VERBOSE(logger, "Producer: starting Enqueue %d\n", j);
		DS_PBQueue_enqueue(queue, i, 10);
	}
	free(i);
	MU_LOG_VERBOSE(logger, "Producer: returning...\n");
	return NULL;
}

void *consumer_timeout_thread(void *args){
	int *i = NULL;
	MU_LOG_VERBOSE(logger, "Consumer: Timed Dequeueing for 5 second timeout!\n");
	if((i = DS_PBQueue_dequeue(queue, 5)) == NULL){
		MU_LOG_VERBOSE(logger, "Consumer: Successfully returned Null!\n");
	}
	MU_LOG_VERBOSE(logger, "Consumer: Timed Dequeueing for 10 second timeout!\n");
	if((i = DS_PBQueue_dequeue(queue, 10))){
		MU_LOG_VERBOSE(logger, "Consumer: Successfully returned %d\n", *i);
	}
	MU_LOG_VERBOSE(logger, "Consumer: returning...\n");
	return NULL;
}

void *producer_thread(void *args){
	int i = 0;
	for(; i < max_num; i++){
		arr[i] = i;
		DS_PBQueue_enqueue(queue, &arr[i], -1);
		if(i % sleep_after){
			sleep(3);
		}
		MU_LOG_VERBOSE(logger, "Enqueued: %d\n", i);
	}
	MU_LOG_VERBOSE(logger, "Producer: returning...\n");
	return NULL;
}

void *consumer_thread(void *args){
	int i = 0;
	for(; i < max_num; i++){
		if(i % sleep_after){
			sleep(5);
		}
		int val = *(int *)DS_PBQueue_dequeue(queue, -1);
		MU_LOG_VERBOSE(logger, "Dequeued: %d\n", val);
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
	logger = MU_Logger_create("./Data_Structures/Logs/DS_PBQueue_Test.log", "w", MU_ALL);
	queue = DS_PBQueue_create(max_size, (void *)compare_integers);
	test_enqueue_and_dequeue();
	DS_PBQueue_clear(queue, print_value);
	test_timed_enqueue_and_dequeue();
	DS_PBQueue_destroy(queue, NULL);
	MU_LOG_VERBOSE(logger, "Finished All Tests!\n");
	MU_Logger_destroy(logger);
	return EXIT_SUCCESS;
}