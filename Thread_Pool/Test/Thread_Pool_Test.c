#include <stdlib.h>
#include <stdio.h>
#include <Thread_Pool.h>
#include <Misc_Utils.h>

int iterations = 0;

static MU_Logger_t *logger;

typedef struct {
	unsigned int task_id;
	size_t amount;
	FILE *logger;
} thread_task;

thread_task *TT_Create(unsigned int thread_id, size_t amount, FILE *logger){
	thread_task *task = malloc(sizeof(thread_task));
	task->task_id = thread_id;
	task->amount = amount;
	task->logger = logger;
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
	sleep(1);
	return retval;
}

void *high_priority_print(char *message){
	int i = 0;
	for(;i<5;i++){
		MU_DEBUG("High priority message: %s\n");
		sleep(1);
	}
	return NULL;
}


void Test_Priority_Tasks(void){
	const unsigned int num_tasks = 10000;
	const int runs = 10;
	//Task *task = TT_Create(1, runs, logger);
	//Result *result = Thread_Pool_Add_Task(print_hello, task, TP_NO_RESULT);
	//assert(!result);
	// TODO: Test other functions.
}

void Test_Non_Timed_Functions(void){
	const unsigned int num_tasks = 60;
	const int runs = 50;
	Result **result = malloc(sizeof(Result *) * num_tasks);
	int i = 0;
	MU_LOG_INFO(logger, "Pausing thread pool to add all tasks!\n");
	Thread_Pool_Pause();
	MU_LOG_INFO(logger, "Thread  Pool Paused to add tasks!\n");
	for(;i<num_tasks ;i++){
		thread_task *task = TT_Create(i+1, runs, logger);
		result[i] = Thread_Pool_Add_Task((void *)print_hello, task, TP_NONE);
	}
	Thread_Pool_Resume();
	MU_LOG_INFO(logger, "All tasks added!\nStarted!\n");
	sleep(5);
	MU_LOG_INFO(logger, "Pausing!\n");
	Thread_Pool_Pause();
	sleep(5);
	Thread_Pool_Resume();
	MU_LOG_INFO(logger, "Resumed!\n");
	MU_LOG_INFO(logger, "Injecting a high priority task!\n");
	Thread_Pool_Add_Task(high_priority_print, "I'm so much better than you!\n", TP_NO_RESULT | TP_HIGH_PRIORITY);
	int retval = *((int *)(Thread_Pool_Obtain_Result(result[num_tasks/2 - 1])));
	MU_ASSERT(retval == num_tasks/2, logger);
	Thread_Pool_Wait();
	MU_LOG_INFO(logger, "Total iterations %d; Should be %d; %s\n" ,iterations, num_tasks * runs
		,iterations == runs * num_tasks ? "True" : "False");
	for(i=0; i<num_tasks; i++){
		free(result[i]->item);
		Thread_Pool_Result_Destroy(result[i]);
	}
	free(result);
}

int main(void){
	logger = malloc(sizeof(MU_Logger_t));
	MU_Logger_Init(logger, "Thread_Pool_Test_Log.txt", "w", MU_ALL);
	// Setup the Thread Pool
	const int num_threads = 5;
	Thread_Pool_Init(num_threads);
	MU_LOG_INFO(logger, "Thread Pool created\n");
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
	Test_Non_Timed_Functions();
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
	MU_LOG_INFO(logger, "Passed All Tests with total time %s!\n", total_formatted_time);
	// Free all resources.
	Timer_Destroy(total_time);
	//Timer_Destroy(timer_one);
	free(total_formatted_time);
	//free(TNTF_Time);
	fclose(logger);
	Thread_Pool_Destroy();
	return EXIT_SUCCESS;
}