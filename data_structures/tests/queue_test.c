#define NO_C_UTILS_PREFIX
#include <pthread.h>

#include "../queue.h"
#include "../../io/logger.h"
#include "../../threading/thread_pool.h"

#include <unistd.h>

static const int num_threads = 4;

static queue_t *queue = NULL;

static logger_t *logger = NULL;

volatile long long int counter = 250000LL;

static void *enqueue_to_queue(void *args) {
	while (counter >= 0) {
		int *i = malloc(sizeof(*i));
		assert(i);

		*i = __sync_sub_and_fetch(&counter, 1);
		
		queue_enqueue(queue, i);
	}
	return NULL;
}

static void *dequeue_from_queue(void *args) {
	while (true) {
		int *i = queue_dequeue(queue);
		
		if (!i) {
			pthread_yield();
			continue;
		}
		else if (*i <= 0) {
			break;
		}

		DEBUG("Dequeued Val: %d", *i);
		
		free(i);
	}
	return NULL;
}

int main(void) {
	logger = logger_create("./data_structures/logs/queue_test.log", "w", LOG_LEVEL_ALL);
	queue = queue_create();
	thread_pool_t *tp = thread_pool_create(num_threads);
	
	for (int i = 1; i <= num_threads; i++)
		thread_pool_add(tp, (i % 2 == 0) ? enqueue_to_queue : dequeue_from_queue, NULL, NO_RESULT);
		
	thread_pool_wait(tp, -1);
	
	thread_pool_destroy(tp);
	logger_destroy(logger);
	queue_destroy(queue, free);
	
	return 0;
}