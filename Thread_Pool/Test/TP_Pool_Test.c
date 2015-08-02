#include <stdlib.h>
#include <stdio.h>
#include <TP_Pool.h>
#include <MU_Logger.h>
#include <unistd.h>

_Atomic int iterations = 0;

static MU_Logger_t *logger;
static TP_Pool_t *tp;
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
		MU_DEBUG("Task %d; Iteration %d;\n", task->task_id, i+1);
		pthread_yield();
	}
	free(task);
	return NULL;
}

int *high_priority_print(char *message){
	int *retval = malloc(sizeof(int));
	int i = 0;
	for(; i < 5; i++){
		MU_DEBUG("%s\n", message);
	}
	*retval = 5;
	return retval;
}

int main(void){
	logger = MU_Logger_create("./Thread_Pool/Logs/TP_Pool_Test.log", "w", MU_ALL);
	tp = TP_Pool_create(pool_size);
	const unsigned int num_tasks = 10000;
	const int runs = 10;
	int i = 0;
	MU_LOG_INFO(logger, "Pausing thread pool to add all tasks...");
	TP_Pool_pause(tp, -1);
	MU_LOG_INFO(logger, "Thread  Pool Paused to add tasks...");
	for(;i<num_tasks ;i++){
		thread_task *task = TT_Create(i+1, runs);
		TP_Pool_add(tp, (void *)print_hello, task, TP_NO_RESULT);
	}
	TP_Pool_resume(tp);
	MU_LOG_INFO(logger, "All tasks added!Starting...");
	sleep(5);
	MU_LOG_INFO(logger, "Pausing for 5 seconds...");
	MU_DEBUG("Pausing for 5 seconds...");
	TP_Pool_pause(tp, 5);
	MU_LOG_INFO(logger, "Injecting a high priority task...");
	TP_Result_t *result = TP_Pool_add(tp, (void *)high_priority_print, "I'm so much better than you...", TP_HIGH_PRIORITY);
	int retval = *(int *)TP_Result_get(result, -1);
	MU_ASSERT((retval == 5), logger, "Bad results from TP_Result_t struct, obtained %d, but expected %d...", retval, 5);
	TP_Result_destroy(result);
	TP_Pool_wait(tp, -1);
	MU_LOG_INFO(logger, "Total iterations %d; Should be %d; %s\n", atomic_load(&iterations), num_tasks * runs
		,iterations == runs * num_tasks ? "True" : "False");
	return EXIT_SUCCESS;
}
