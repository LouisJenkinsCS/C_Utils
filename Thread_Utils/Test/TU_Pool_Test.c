#define C_UTILS_USE_POSIX_STD
#include <stdlib.h>
#include <stdio.h>
#include <TP_Pool.h>
#include <MU_Logger.h>
#include <unistd.h>

static _Atomic int iterations = 0;

static logger_t *logger;
static pool_t *tp;
static const int pool_size = 5;

typedef struct {
	unsigned int task_id;
	size_t amount;
} thread_task;

thread_task *TT_Create(unsigned int thread_id, size_t amount){
	thread_task *task = malloc(sizeof(thread_task));
	task->task_id = thread_id;
	task->amount = amount;
	return task;
}

void *print_hello(thread_task *task){
	int i = 0;
	for(; i<task->amount; i++) {
		atomic_fetch_add(&iterations, 1);
		DEBUG("Task %d; Iteration %d;\n", task->task_id, i+1);
		pthread_yield();
	}
	free(task);
	return NULL;
}

int *high_priority_print(char *message){
	int *retval = malloc(sizeof(int));
	int i = 0;
	for(; i < 5; i++){
		DEBUG("%s\n", message);
	}
	*retval = 5;
	return retval;
}

int main(void){
	logger = logger_create("./Thread_Utils/Logs/TU_Pool_Test.log", "w", MU_ALL);
	tp = pool_create(pool_size);
	const unsigned int num_tasks = 10000;
	const int runs = 10;
	int i = 0;
	LOG_INFO(logger, "Pausing thread pool to add all tasks...");
	pool_pause(tp, -1);
	LOG_INFO(logger, "Thread  Pool Paused to add tasks...");
	for(;i<num_tasks ;i++){
		thread_task *task = TT_Create(i+1, runs);
		pool_add(tp, (void *)print_hello, task, TP_NO_RESULT);
	}
	pool_resume(tp);
	LOG_INFO(logger, "All tasks added!Starting...");
	sleep(5);
	LOG_INFO(logger, "Pausing for 5 seconds...");
	DEBUG("Pausing for 5 seconds...");
	pool_pause(tp, 5);
	LOG_INFO(logger, "Injecting a high priority task...");
	result_t *result = pool_add(tp, (void *)high_priority_print, "I'm so much better than you...", TP_HIGH_PRIORITY);
	int retval = *(int *)result_get(result, -1);
	ASSERT((retval == 5), logger, "Bad results from result_t struct, obtained %d, but expected %d...", retval, 5);
	result_destroy(result);
	pool_wait(tp, -1);
	LOG_INFO(logger, "Total iterations %d; Should be %d; %s\n", atomic_load(&iterations), num_tasks * runs
		,iterations == runs * num_tasks ? "True" : "False");
	return EXIT_SUCCESS;
}
