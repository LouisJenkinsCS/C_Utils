#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NO_C_UTILS_PREFIX
#include "../priority_queue.h"
#include "../../io/logger.h"

static const int max_size = 5;
static const int max_num = 10;
static const int sleep_after = 5;
static int arr[10];

static logger_t *logger;
static priority_queue_t *queue;

void print_value(void *number) {
	LOG_VERBOSE(logger, "Queue has value: %d\n", *(int *)number);

	free(number);
}

void *producer_timeout_thread(void *args) {
	int *i = malloc(sizeof(int));
	assert(i);
	*i = 0;

	LOG_VERBOSE(logger, "Producer: sleeping for 10 seconds...\n");
	sleep(10);
	
	LOG_VERBOSE(logger, "Producer: woke up, Enqueueing now!\n");
	priority_queue_enqueue(queue, i, 5);
	
	LOG_VERBOSE(logger, "Producer: sleeping for 10 seconds...\n");
	sleep(10);
	
	LOG_VERBOSE(logger, "Producer: woke up, timed enqueueing %d times for 10 second timeout!\n", max_size);
	for (int j = 0;j <= max_size; j++) {
		LOG_VERBOSE(logger, "Producer: starting Enqueue %d\n", j);
		priority_queue_enqueue(queue, i, 10);
	}
	free(i);

	C_UTILS_LOG_VERBOSE(logger, "Producer: returning...\n");
	
	return NULL;
}

void *consumer_timeout_thread(void *args) {
	int *i = NULL;
	
	C_UTILS_LOG_VERBOSE(logger, "Consumer: Timed Dequeueing for 5 second timeout!\n");
	if ((i = priority_queue_dequeue(queue, 5)) == NULL)
		C_UTILS_LOG_VERBOSE(logger, "Consumer: Successfully returned Null!\n");

	C_UTILS_LOG_VERBOSE(logger, "Consumer: Timed Dequeueing for 10 second timeout!\n");
	if ((i = priority_queue_dequeue(queue, 10)))
		C_UTILS_LOG_VERBOSE(logger, "Consumer: Successfully returned %d\n", *i);

	C_UTILS_LOG_VERBOSE(logger, "Consumer: returning...\n");
	
	return NULL;
}

void *producer_thread(void *args) {
	for (int i = 0; i < max_num; i++) {
		arr[i] = i;
		priority_queue_enqueue(queue, &arr[i], -1);
		
		if (i % sleep_after)
			sleep(3);

		C_UTILS_LOG_VERBOSE(logger, "Enqueued: %d\n", i);
	}
	
	C_UTILS_LOG_VERBOSE(logger, "Producer: returning...\n");
	
	return NULL;
}

void *consumer_thread(void *args) {
	for (int i = 0; i < max_num; i++) {
		if (i % sleep_after)
			sleep(5);

		int val = *(int *)priority_queue_dequeue(queue, -1);
		C_UTILS_LOG_VERBOSE(logger, "Dequeued: %d\n", val);
	}

	C_UTILS_LOG_VERBOSE(logger, "Consumer: returning...\n");
	
	return NULL;
}

int compare_integers(void *item_one, void *item_two) {
	return *((int *)item_one) - *((int *)item_two);
}

void test_enqueue_and_dequeue(void) {
	pthread_t** threads = malloc(sizeof(pthread_t) * 2);
	
	threads[0] = malloc(sizeof(pthread_t));
	threads[1] = malloc(sizeof(pthread_t));
	
	pthread_create(threads[0], NULL, producer_thread, queue);
	pthread_create(threads[1], NULL, consumer_thread, queue);
	
	sleep(30);
	void *retval = NULL;

	pthread_join(*(threads[0]), &retval);
	pthread_join(*(threads[1]), &retval);
	
	free(threads[0]);
	free(threads[1]);
	free(threads);
	
	C_UTILS_LOG_INFO(logger, "Finished Enqueue_and_Dequeue!\n");
}

void test_timed_enqueue_and_dequeue(void) {
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
	
	C_UTILS_LOG_VERBOSE(logger, "Finished Timed_Enqueue_and_Dequeue!\n");
}

int main(void) {
	logger = logger_create("./data_structures/logs/priority_queue_test.log", "w", LOG_LEVEL_ALL);
	queue = priority_queue_create(max_size, (void *)compare_integers);
	
	test_enqueue_and_dequeue();
	priority_queue_clear(queue, print_value);
	
	test_timed_enqueue_and_dequeue();
	priority_queue_destroy(queue, NULL);
	
	C_UTILS_LOG_VERBOSE(logger, "Finished All Tests!\n");
	logger_destroy(logger);
	
	return EXIT_SUCCESS;
}