#include <stdlib.h>
#include <stdio.h>
#include "Thread_Pool.h"

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

void print_hello(thread_task *task){
	int i = 0;
	for(; i<task->amount; i++)
		printf("Thread %d; Iteration %d;\nHello World!", task->thread_id, i);
	free(task);
}

int main(void){
	const int runs = 1000;
	Thread_Pool *tp = Thread_Pool_Create(10, NONE);
	Result **result = malloc(sizeof(Result) * runs);
	int i = 0;
	for(;i<runs;i++){
		thread_task *task = TT_Create(i+1, 10);
		result[i] = TP_Add_Task(tp, (void *)print_hello, task);
	}
	TP_Obtain_Result(result[runs-1]);
	return EXIT_SUCCESS;
}