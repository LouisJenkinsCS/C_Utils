#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "Thread_Pool.h"


typedef struct{
	time_t *start;
	time_t *end;
} Timer_T;

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
		//printf("Task %d; Iteration %d;\n", task->task_id, i+1);
		pthread_yield();
	}
	free(task);
	return retval;
}

Timer_T *Timer_Init(void){
	Timer_T *timer = malloc(sizeof(Timer_T));
	timer->start = malloc(sizeof(time_t));
	timer->end = malloc(sizeof(time_t));
	return timer;
}

void Timer_Start(Timer_T *timer){
	time(timer->start);
}

void Timer_Stop(Timer_T *timer){
	time(timer->end);
}

char *Timer_Get_Formatted(Timer_T *timer){
	double total_time = difftime(*timer->end, *timer->start);
	int hours = 0, minutes = 0, seconds = 0;
	while((int)(total_time/3600)){
		hours++;
		total_time -= 3600;
	}
	while((int)(total_time/60)){
		minutes++;
		total_time -= 60;
	}
	while((int)total_time){
		seconds++;
		total_time--;
	}
	char *formatted_time;
	asprintf(&formatted_time, "%02d:%02d:%02d", hours, minutes, seconds);
	return formatted_time;
}

void Timer_Destroy(Timer_T *timer){
	free(timer->start);
	free(timer->end);
	free(timer);
}

void Test_Non_Timed_Functions(void){
	const unsigned int num_tasks = 10000;
	const int runs = 50;
	printf("Thread Pool created\n");
	Result **result = malloc(sizeof(Result *) * num_tasks);
	int i = 0;
	printf("Pausing thread pool to add all tasks!\n");
	Thread_Pool_Pause();
	printf("Thread  Pool Paused to add tasks!\n");
	for(;i<num_tasks ;i++){
		thread_task *task = TT_Create(i+1, runs);
		result[i] = Thread_Pool_Add_Task((void *)print_hello, task, TP_NONE);
	}
	Thread_Pool_Resume();
	printf("All tasks added!\nStarted!\n");
	sleep(5);
	printf("Pausing!\n");
	Thread_Pool_Pause();
	sleep(10);
	Thread_Pool_Resume();
	printf("Resumed!\n");
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
}

int main(void){
	// Setup the Thread Pool
	const int num_threads = 5;
	Thread_Pool_Init(num_threads);
	// Setup the timers for each test.
	Timer_T *total_time = Timer_Init();
	Timer_T *timer_one = Timer_Init();
	Timer_T *timer_two = Timer_Init();
	Timer_T *timer_three = Timer_Init();
	// Begin the timer to calculate the total timer as well as the first task.
	Timer_Start(total_time);
	Timer_Start(timer_one);
	Timer_Start(timer_two);
	Timer_Start(timer_three);
	// Begin all tests and timers below.
	Test_Non_Timed_Functions();
	Timer_Stop(timer_one);
	Timer_Start(timer_two);
	Test_Priority_Tasks();
	Timer_Stop(timer_two);
	Timer_Start(timer_three);
	Test_Timed_Functions();
	Timer_Stop(timer_three);
	Timer_Stop(total_time);
	// All tests being finished, calculate individual and total time.
	char *TNTF_Time = Timer_Get_Formatted(timer_one);
	char *TPT_Time = Timer_Get_Formatted(timer_two);
	char *TTF_Time = Timer_Get_Formatted(timer_three);
	char *total_formatted_time = Timer_Get_Formatted(total_time);
	// Print all test times.
	printf("Passed Test_Non_Timed_Functions with time of: %s!\n", TNTF_Time);
	printf("Passed Test_Priority_Tasks with time of %s!\n", TPT_Time);
	printf("Passed Test_Timed_Functions with time of %s!\n", TTF_Time);
	printf("Passed All Tests with total time %s!\n", total_formatted_time);
	// Free all resources.
	Timer_Destroy(total_time);
	Timer_Destroy(timer_one);
	free(total_formatted_time);
	free(TNTF_Time);
	Thread_Pool_Destroy();
	return EXIT_SUCCESS;
}