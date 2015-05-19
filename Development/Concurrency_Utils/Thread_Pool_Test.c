#include <stdlib.h>
#include <stdio.h>
#include "Thread_Pool.h"

int iterations = 0;

typedef struct {
	unsigned int thread_id;
	size_t amount;
} thread_task;

thread_task *TT_Create(unsigned int thread_id, size_t amount){
	thread_task *task = malloc(sizeof(thread_task));
	task->thread_id = thread_id;
	task->amount = amount;
	return task;
}

void *print_hello(thread_task *task){
	int i = 0;
	for(; i<task->amount; i++) {
		iterations++;
		printf("Thread %d; Iteration %d;\nHello World!", task->thread_id, i);
		pthread_yield();
	}
	free(task);
	return NULL;
}

int main(void){
	const int num_threads = 10;
	const int runs = 1000;
	Thread_Pool *tp = TP_Create(num_threads, NONE);
	printf("Thread Pool created\n");
	Result **result = malloc(sizeof(Result) * runs);
	int i = 0;
	for(;i<num_threads;i++){
		thread_task *task = TT_Create(i+1, runs);
		result[i] = TP_Add_Task(tp, (void *)print_hello, task);
	}
	printf("All threads started!\n");
	//TP_Obtain_Result(result[runs-2]);
	sleep(5);
	printf("Total iterations %d; Should be %d; %s\n" ,iterations, num_threads * runs
		,iterations == runs * num_threads ? "True" : "False");
	return EXIT_SUCCESS;
}