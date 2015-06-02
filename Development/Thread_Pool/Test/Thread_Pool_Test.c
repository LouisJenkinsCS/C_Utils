#include <stdlib.h>
#include <stdio.h>
#include <Thread_Pool.h>
#include <Misc_Utils.h>

int iterations = 0;

typedef struct {
	unsigned int task_id;
	size_t amount;
	FILE *fp;
} thread_task;

thread_task *TT_Create(unsigned int thread_id, size_t amount, FILE *fp){
	thread_task *task = malloc(sizeof(thread_task));
	task->task_id = thread_id;
	task->amount = amount;
	task->fp = fp;
	return task;
}

int *print_hello(thread_task *task){
	int *retval = malloc(sizeof(int *));
	*retval = task->task_id;
	int i = 0;
	for(; i<task->amount; i++) {
		iterations++;
		MU_DEBUG("Task %d; Iteration %d;\n", task->task_id, i+1);
		pthread_yield();
	}
	free(task);
	return retval;
}


void Test_Priority_Tasks(void){
	const unsigned int num_tasks = 10000;
	const int runs = 10;
	//Task *task = TT_Create(1, runs, fp);
	//Result *result = Thread_Pool_Add_Task(print_hello, task, TP_NO_RESULT);
	//assert(!result);
	// TODO: Test other functions.
}

void Test_Non_Timed_Functions(FILE *fp){
	const unsigned int num_tasks = 10000;
	const int runs = 50;
	Result **result = malloc(sizeof(Result *) * num_tasks);
	int i = 0;
	MU_LOG_INFO(fp, "Pausing thread pool to add all tasks!\n");
	Thread_Pool_Pause();
	MU_LOG_INFO(fp, "Thread  Pool Paused to add tasks!\n");
	for(;i<num_tasks ;i++){
		thread_task *task = TT_Create(i+1, runs, fp);
		result[i] = Thread_Pool_Add_Task((void *)print_hello, task, TP_NONE);
	}
	Thread_Pool_Resume();
	MU_LOG_INFO(fp, "All tasks added!\nStarted!\n");
	sleep(5);
	MU_LOG_INFO(fp, "Pausing!\n");
	Thread_Pool_Pause();
	sleep(10);
	Thread_Pool_Resume();
	MU_LOG_INFO(fp, "Resumed!\n");
	int retval = *((int *)(Thread_Pool_Obtain_Result(result[num_tasks/2 - 1])));
	MU_ASSERT(retval == num_tasks/2, fp);
	Thread_Pool_Wait();
	MU_LOG_INFO(fp, "Total iterations %d; Should be %d; %s\n" ,iterations, num_tasks * runs
		,iterations == runs * num_tasks ? "True" : "False");
	for(i=0; i<num_tasks; i++){
		free(result[i]->item);
		Thread_Pool_Result_Destroy(result[i]);
	}
	free(result);
}

int main(void){
	FILE *fp = fopen("Thread_Pool_Test_Log.txt", "w");
	// Setup the Thread Pool
	const int num_threads = 5;
	Thread_Pool_Init(num_threads);
	MU_LOG_INFO(fp, "Thread Pool created\n");
	// Setup the timers for each test.
	Timer_t *total_time = Timer_Init(1);
	//Timer_t *timer_one = Timer_Init();
	//Timer_T *timer_two = Timer_Init();
	//Timer_T *timer_three = Timer_Init();
	// Begin the timer to calculate the total timer as well as the first task.
	//Timer_Start(total_time);
	//Timer_Start(timer_one);
	//Timer_Start(timer_two);
	//Timer_Start(timer_three);
	// Begin all tests and timers below.
	Test_Non_Timed_Functions(fp);
	//Timer_Stop(timer_one);
	//Timer_Start(timer_two);
	//Test_Priority_Tasks();
	//Timer_Stop(timer_two);
	//Timer_Start(timer_three);
	//Test_Timed_Functions();
	//Timer_Stop(timer_three);
	Timer_Stop(total_time);
	// All tests being finished, calculate individual and total time.
	//char *TNTF_Time = Timer_Get_Formatted(timer_one);
	//char *TPT_Time = Timer_Get_Formatted(timer_two);
	//char *TTF_Time = Timer_Get_Formatted(timer_three);
	char *total_formatted_time = Timer_To_String(total_time);
	// Print all test times.
	//printf("Passed Test_Non_Timed_Functions with time of: %s!\n", TNTF_Time);
	//printf("Passed Test_Priority_Tasks with time of %s!\n", TPT_Time);
	//printf("Passed Test_Timed_Functions with time of %s!\n", TTF_Time);
	MU_LOG_INFO(fp, "Passed All Tests with total time %s!\n", total_formatted_time);
	// Free all resources.
	Timer_Destroy(total_time);
	//Timer_Destroy(timer_one);
	free(total_formatted_time);
	//free(TNTF_Time);
	fclose(fp);
	Thread_Pool_Destroy();
	return EXIT_SUCCESS;
}