#define NO_C_UTILS_PREFIX
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>

#include "../thread_pool.h"
#include "../../io/logger.h"

static _Atomic int iterations = 0;
static logger_t *logger;
static thread_pool_t *tp;
static const int pool_size = 5;

LOGGER_AUTO_CREATE(logger, "./threading/logs/thread_pool_test.log", "w", LOG_LEVEL_ALL);

struct c_utils_test_thread_task{
	unsigned int task_id;
	size_t amount;
};

static struct c_utils_test_thread_task *create_task(unsigned int thread_id, size_t amount) {
	struct c_utils_test_thread_task *task = malloc(sizeof(struct c_utils_test_thread_task));
	
	task->task_id = thread_id;
	task->amount = amount;
	
	return task;
}

static void *print_hello(struct c_utils_test_thread_task *task) {
	for (int i = 0; i<task->amount; i++) {
		atomic_fetch_add(&iterations, 1);
		DEBUG("Task %d; Iteration %d;\n", task->task_id, i+1);
		
		pthread_yield();
	}
	
	free(task);
	
	return NULL;
}

static int *high_priority_print(char *message) {
	int *retval = malloc(sizeof(int));
	
	for (int i = 0; i < 5; i++)
		DEBUG("%s\n", message);

	*retval = 5;
	return retval;
}

int main(void) {
	tp = thread_pool_create(pool_size);
	const unsigned int num_tasks = 10000;
	const int runs = 10;

	LOG_INFO(logger, "Pausing thread pool to add all tasks...");
	thread_pool_pause_for(tp, C_UTILS_THREAD_POOL_NO_TIMEOUT);
	
	LOG_INFO(logger, "Thread  Pool Paused to add tasks...");
	for (int i = 0;i<num_tasks ;i++)
		thread_pool_add(tp, (void *)print_hello, create_task(i+1, runs), NO_RESULT);

	thread_pool_resume(tp);
	
	LOG_INFO(logger, "All tasks added!Starting...");
	sleep(5);
	
	LOG_INFO(logger, "Pausing for 5 seconds...");
	DEBUG("Pausing for 5 seconds...");
	thread_pool_pause_for(tp, 5 * C_UTILS_THREAD_POOL_SECOND);
	
	LOG_INFO(logger, "Injecting a high priority task...");
	result_t *result = thread_pool_add(tp, (void *)high_priority_print, "I'm so much better than you...", HIGH_PRIORITY);
	int retval = *(int *)result_get(result, -1);
	ASSERT((retval == 5), logger, "Bad results from result_t struct, obtained %d, but expected %d...", retval, 5);
	
	result_destroy(result);
	thread_pool_wait_for(tp, C_UTILS_THREAD_POOL_NO_TIMEOUT);
	LOG_INFO(logger, "Total iterations %d; Should be %d; %s\n", atomic_load(&iterations), num_tasks * runs
		,iterations == runs * num_tasks ? "True" : "False");
	
	return EXIT_SUCCESS;
}
