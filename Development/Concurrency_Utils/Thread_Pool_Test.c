#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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
		//printf("Task %d; Iteration %d;\nHello World!", task->thread_id, i);
		pthread_yield();
	}
	free(task);
	return NULL;
}

char *format_time(double time){
	int hours = 0, minutes = 0, seconds = 0;
	while((int)(time/3600)){
		hours++;
		time -= 3600;
	}
	while((int)(time/60)){
		minutes++;
		time -= minutes;
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
	const int num_threads = 10;
	const unsigned int num_tasks = 1000;
	const int runs = 1;
	time_t *start = malloc(sizeof(time_t));
	time_t *end = malloc(sizeof(time_t));
	time(start);
	Thread_Pool *tp = TP_Create(num_threads, NONE);
	printf("Thread Pool created\n");
	Result **result = malloc(sizeof(Result) * num_tasks);
	int i = 0;
	for(;i<num_tasks ;i++){
		thread_task *task = TT_Create(i+1, runs);
		result[i] = TP_Add_Task(tp, (void *)print_hello, task);
	}
	printf("All tasks added!\n");
	TP_Wait(tp);
	printf("Total iterations %d; Should be %d; %s\n" ,iterations, num_tasks * runs
		,iterations == runs * num_tasks ? "True" : "False");
	for(i=0; i<num_tasks; i++){
		TP_Result_Destroy(result[i]);
	}
	free(result);
	TP_Destroy(tp);
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