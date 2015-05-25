#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "Thread_Pool.h"

int iterations = 0;

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

int *print_hello(thread_task *task){
	int *retval = malloc(sizeof(int *));
	*retval = task->task_id;
	int i = 0;
	for(; i<task->amount; i++) {
		iterations++;
		printf("Task %d; Iteration %d;\n", task->task_id, i+1);
		pthread_yield();
	}
	free(task);
	return retval;
}

char *format_time(double time){
	int hours = 0, minutes = 0, seconds = 0;
	while((int)(time/3600)){
		hours++;
		time -= 3600;
	}
	while((int)(time/60)){
		minutes++;
		time -= 60;
	}
	while((int)time){
		seconds++;
		time--;
	}
	char *formatted_time;
	asprintf(&formatted_time, "%02d:%02d:%02d", hours, minutes, seconds);
	return formatted_time;
}

int main(void){
	const int num_threads = 2;
	const unsigned int num_tasks = 1000;
	const int runs = 50;
	time_t *start = malloc(sizeof(time_t));
	time_t *end = malloc(sizeof(time_t));
	time(start);
	Thread_Pool_Init(num_threads);
	printf("Thread Pool created\n");
	Result **result = malloc(sizeof(Result *) * num_tasks);
	int i = 0;
	for(;i<num_tasks ;i++){
		thread_task *task = TT_Create(i+1, runs);
		result[i] = Thread_Pool_Add_Task((void *)print_hello, task, TP_NONE);
	}
	printf("All tasks added!\nStarted!\n");
	sleep(1);
	printf("Pausing!\n");
	Thread_Pool_Pause();
	sleep(10);
	Thread_Pool_Resume();
	int retval = *((int *)(Thread_Pool_Obtain_Result(result[num_tasks/2 - 1])));
	assert(retval == num_tasks/2);
	Thread_Pool_Wait();
	printf("Total iterations %d; Should be %d; %s\n" ,iterations, num_tasks * runs
		,iterations == runs * num_tasks ? "True" : "False");
	for(i=0; i<num_tasks; i++){
		free(result[i]->item);
		Thread_Pool_Result_Destroy(result[i]);
	}
	free(result);
	Thread_Pool_Destroy();
	printf("Thread_Pool struct destroyed!\n");
	time(end);
	double total_time = difftime(*end, *start);
	char *formatted_time = format_time(total_time);
	printf("Total time is %s\n", formatted_time);
	free(formatted_time);
	free(start);
	free(end);
	return EXIT_SUCCESS;
}